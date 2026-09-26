#include "VDX7UserBank.h"
#include "VDX7Sysex.h"
#include <iostream>
#include <stdexcept>
#include <thread>
#include <atomic>

static void require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

static uint32_t testChecksum(const uint8_t* data, size_t size)
{
    uint32_t crc = 0xffffffffu;
    for (size_t i = 0; i < size; ++i)
    {
        crc ^= data[i];
        for (int bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ ((crc & 1) ? 0xedb88320u : 0u);
    }
    return ~crc;
}

static void setLittleEndian32(uint8_t* data, uint32_t value)
{
    for (int i = 0; i < 4; ++i)
        data[i] = static_cast<uint8_t>(value >> (8 * i));
}

int main(int argc, char** argv)
{
    // A separate process is needed: POSIX file locks may be process-scoped.
    if (argc == 3 && juce::String(argv[1]) == "--hold-lock")
    {
        juce::InterProcessLock lock(VDX7UserBank::lockName);
        if (!lock.enter(0)) return 2;
        juce::File ready(argv[2]);
        if (!ready.replaceWithText("ready")) { lock.exit(); return 3; }
        const auto release = ready.withFileExtension(".release");
        for (int i = 0; i < 1000 && !release.existsAsFile(); ++i) juce::Thread::sleep(5);
        lock.exit();
        return 0;
    }
    juce::TemporaryFile temporary;
    const auto directory = temporary.getFile();
    struct Cleanup { juce::File directory; ~Cleanup() { directory.deleteRecursively(); } } cleanup { directory };
    try
    {
        require(directory.createDirectory().wasOk(), "test directory");
        const auto file = directory.getChildFile("USER.vdxbank");
        VDX7UserBank::Snapshot empty;
        require(VDX7UserBank::load(file, empty).wasOk() && !empty.exists && empty.occupiedMask == 0,
                "missing library is empty without creating a file");
        require(!file.exists(), "reading missing bank does not write it");
        for (int slot = 0; slot < 32; ++slot)
            require(!empty.occupied(slot) && empty.name(slot) == "EMPTY", "silent placeholder slots");

        auto patch = empty.voices[0];
        patch[16] = 85; // Synthetic test voice, not factory data.
        require(VDX7UserBank::savePatch(file, empty, 3, patch, "MY PATCH", false).wasOk(), "first patch save");
        VDX7UserBank::Snapshot first;
        require(VDX7UserBank::load(file, first).wasOk() && first.exists && first.occupied(3), "reload from disk");
        require(first.name(3) == "MY PATCH" && first.voices[3][16] == 85, "patch/name persisted");
        for (int slot = 0; slot < 32; ++slot)
            if (slot != 3) require(first.voices[slot] == empty.voices[slot], "other slots preserved");

        // A second instance used the older empty snapshot. Different slot must merge.
        patch[16] = 42;
        require(VDX7UserBank::savePatch(file, empty, 31, patch, "LAST", false).wasOk(), "merge disjoint stale save");
        VDX7UserBank::Snapshot second;
        require(VDX7UserBank::load(file, second).wasOk(), "reload merged bank");
        require(second.voices[3] == first.voices[3] && second.occupied(31), "both instance saves survive");
        require(second.occupiedMask == ((uint32_t(1) << 3) | (uint32_t(1) << 31)), "slot 32 occupancy");
        require(VDX7UserBank::savePatch(file, second, 3, patch, "REPLACE", false).failed(), "overwrite requires confirmation");
        require(VDX7UserBank::savePatch(file, empty, 3, patch, "REPLACE", true).failed(), "stale confirmation rejected");
        require(VDX7UserBank::savePatch(file, second, 3, patch, "REPLACE", true).wasOk(), "confirmed current overwrite");
        require(VDX7UserBank::savePatch(file, second, 3, patch, "STALE", true).failed(), "changed occupied slot conflict");

        juce::MemoryBlock goodBytes;
        require(file.loadFileAsData(goodBytes), "capture valid file");
        VDX7UserBank::Snapshot validSnapshot;
        require(VDX7UserBank::load(file, validSnapshot).wasOk(), "capture valid bank snapshot");

        // CRC-valid files still have to obey packed voice semantics. Change
        // occupied slot 04, recompute CRC, and ensure failure is transactional.
        for (const auto& invalidField : std::array<std::pair<int, uint8_t>, 6> {{
                 {0, 100},       // operator rate
                 {14, 100},      // operator output level
                 {16, 100},      // operator fine frequency
                 {12, 0x78},     // detune nibble 15 (+8)
                 {116, 0x0c},    // LFO waveform 6
                 {117, 49}       // transpose beyond +24
             }})
        {
            auto malformed = goodBytes;
            auto* bytes = static_cast<uint8_t*>(malformed.getData());
            bytes[8 + 3 * 128 + invalidField.first] = invalidField.second;
            setLittleEndian32(bytes + malformed.getSize() - 4,
                              testChecksum(bytes, malformed.getSize() - 4));
            require(file.replaceWithData(bytes, malformed.getSize()), "write semantic-invalid fixture");
            auto destination = validSnapshot;
            require(VDX7UserBank::load(file, destination).failed(),
                    "reject CRC-valid USER bank with invalid packed voice field");
            require(destination.voices == validSnapshot.voices
                    && destination.occupiedMask == validSnapshot.occupiedMask,
                    "semantic-invalid USER bank leaves caller snapshot unchanged");
        }

        // Occupied patch names are printable ASCII, just like names accepted
        // by savePatch; control characters must not enter the GUI via import.
        {
            auto malformed = goodBytes;
            auto* bytes = static_cast<uint8_t*>(malformed.getData());
            bytes[8 + 3 * 128 + 118] = '\n';
            setLittleEndian32(bytes + malformed.getSize() - 4,
                              testChecksum(bytes, malformed.getSize() - 4));
            require(file.replaceWithData(bytes, malformed.getSize()), "write invalid-name fixture");
            auto destination = validSnapshot;
            require(VDX7UserBank::load(file, destination).failed(), "reject control character in occupied USER name");
            require(destination.voices == validSnapshot.voices
                    && destination.occupiedMask == validSnapshot.occupiedMask,
                    "invalid USER name leaves caller snapshot unchanged");
        }
        require(file.replaceWithData(goodBytes.getData(), goodBytes.getSize()), "restore valid bank after semantic probes");
        for (int slot : { -1, 32 })
            require(VDX7UserBank::savePatch(file, second, slot, patch, "INVALID", true).failed(), "reject invalid slots");
        for (const auto& name : { juce::String(), juce::String("           "), juce::String("12345678901"),
                                 juce::String("\n"), juce::String::fromUTF8("\xc3\xa1") })
            require(VDX7UserBank::savePatch(file, second, 0, patch, name, false).failed(), "reject invalid names");
        auto invalid = patch;
        invalid[0] = 128;
        require(VDX7UserBank::savePatch(file, second, 0, invalid, "INVALID", false).failed(), "reject invalid voice byte");
        invalid = patch;
        invalid[0] = 100;
        require(VDX7UserBank::savePatch(file, second, 0, invalid, "INVALID", false).failed(),
                "reject semantically invalid packed voice before writing");
        require(VDX7UserBank::savePatch(directory.getChildFile("missing/USER.vdxbank"), empty,
                    0, patch, "NO FOLDER", false).failed(), "failed I/O does not create arbitrary folders");
        juce::MemoryBlock afterInvalid;
        require(file.loadFileAsData(afterInvalid) && afterInvalid == goodBytes, "validation failures preserve file");

        // Format/version, truncation, checksum damage: no silent reset or overwrite.
        for (int corruption = 0; corruption < 3; ++corruption)
        {
            auto broken = goodBytes;
            if (corruption == 0) static_cast<uint8_t*>(broken.getData())[3] = '2';
            if (corruption == 1) broken.setSize(100);
            if (corruption == 2) static_cast<uint8_t*>(broken.getData())[64] ^= 1;
            require(file.replaceWithData(broken.getData(), broken.getSize()), "corrupt fixture");
            auto unchanged = first;
            require(VDX7UserBank::load(file, unchanged).failed(), "corrupt bank rejected");
            require(unchanged.voices == first.voices && unchanged.occupiedMask == first.occupiedMask,
                    "failed load preserves caller snapshot");
            require(VDX7UserBank::savePatch(file, empty, 0, patch, "NO RESET", false).failed(), "never overwrite damaged bank");
            juce::MemoryBlock preserved;
            require(file.loadFileAsData(preserved) && preserved == broken, "damaged original kept for recovery");
        }
        require(file.replaceWithData(goodBytes.getData(), goodBytes.getSize()), "restore test fixture");

        // Held lock in another host/process must prevent any mutation.
        const auto ready = directory.getChildFile("ready");
        juce::ChildProcess child;
        require(child.start(juce::StringArray { juce::File::getSpecialLocation(juce::File::currentExecutableFile).getFullPathName(),
                                               "--hold-lock", ready.getFullPathName() }), "start lock holder");
        for (int i = 0; i < 200 && !ready.existsAsFile(); ++i) juce::Thread::sleep(5);
        require(ready.existsAsFile(), "child owns lock");
        require(VDX7UserBank::savePatch(file, empty, 0, patch, "BUSY", false).failed(), "cross-process busy protection");
        require(ready.withFileExtension(".release").replaceWithText("release"), "release child lock");
        require(child.waitForProcessToFinish(3000) && child.getExitCode() == 0, "lock holder exits");
        afterInvalid.reset(); // File::loadFileAsData appends to a MemoryBlock.
        require(file.loadFileAsData(afterInvalid) && afterInvalid == goodBytes, "busy save preserves file");
        require(VDX7UserBank::savePatch(file, empty, 0, patch, "AFTER LOCK", false).wasOk(), "retry after unlock");

        // Packed voices remain compatible with the existing bank SysEx exporter.
        VDX7UserBank::Snapshot final;
        require(VDX7UserBank::load(file, final).wasOk(), "final reload");
        std::vector<uint8_t> packed;
        for (const auto& voice : final.voices) packed.insert(packed.end(), voice.begin(), voice.end());
        std::vector<uint8_t> decoded;
        const auto sysex = VDX7Sysex::encode(packed);
        require(sysex.size() == 4104 && VDX7Sysex::decode(sysex, decoded) && decoded == packed,
                "complete bank SysEx round trip");
        for (int slot : {0, 3, 31})
        {
            std::vector<uint8_t> voice(final.voices[slot].begin(), final.voices[slot].end());
            require(VDX7Sysex::decode(VDX7Sysex::encode(voice), decoded) && decoded == voice,
                    "individual patch SysEx round trip");
        }
        // Two instances within one host can save simultaneously. POSIX locking
        // alone is insufficient for same-process writers; exercise the mutex.
        std::atomic<bool> start { false };
        std::array<bool, 2> saved {};
        const auto writer = [&](int writerIndex)
        {
            while (!start.load()) std::this_thread::yield();
            for (int attempt = 0; attempt < 100; ++attempt)
            {
                const auto result = VDX7UserBank::savePatch(file, final, 10 + writerIndex,
                    patch, writerIndex == 0 ? "THREAD A" : "THREAD B", false);
                if (result.wasOk()) { saved[writerIndex] = true; return; }
                if (!result.getErrorMessage().contains("busy")) return;
                juce::Thread::sleep(2);
            }
        };
        std::thread firstWriter(writer, 0), secondWriter(writer, 1);
        start.store(true);
        firstWriter.join(); secondWriter.join();
        require(saved[0] && saved[1], "same-process concurrent updates succeed after busy retry");
        VDX7UserBank::Snapshot concurrent;
        require(VDX7UserBank::load(file, concurrent).wasOk(), "reload concurrent saves");
        require(concurrent.name(10) == "THREAD A" && concurrent.name(11) == "THREAD B",
                "both concurrent slots retained");
        for (int slot = 0; slot < 32; ++slot)
            if (slot != 10 && slot != 11)
                require(concurrent.voices[slot] == final.voices[slot], "concurrent saves preserve other slots");
        require(file.deleteFile(), "delete test fixture only");
        require(VDX7UserBank::savePatch(file, final, 3, patch, "NO RECREATE", true).failed(), "deleted bank conflict");
        require(!file.exists(), "deleted bank not silently recreated from stale view");
        std::cout << "PASS: persistent USER slots, merge/conflict/overwrite policy, failed I/O, corruption, "
                     "cross-process lock and single/bank SysEx round trips (no ROM)\n";
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
