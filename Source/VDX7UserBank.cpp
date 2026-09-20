#include "VDX7UserBank.h"
#include <algorithm>
#include <mutex>
#include <vector>

namespace
{
constexpr size_t payloadSize = 4 + 4 + 32 * 128;
constexpr size_t fileSize = payloadSize + 4;
std::mutex localWriterMutex;

uint32_t read32(const uint8_t* p)
{
    return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}
void write32(uint8_t* p, uint32_t v)
{
    for (int i = 0; i < 4; ++i) p[i] = static_cast<uint8_t>(v >> (8 * i));
}
uint32_t checksum(const uint8_t* p, size_t count)
{
    uint32_t crc = 0xffffffffu;
    for (size_t i = 0; i < count; ++i)
    {
        crc ^= p[i];
        for (int bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ ((crc & 1) ? 0xedb88320u : 0u);
    }
    return ~crc;
}
juce::Result fail(const char* message) { return juce::Result::fail(message); }
}

VDX7UserBank::Snapshot::Snapshot()
{
    // Silent, seven-bit placeholder voices. Occupancy is separate from names.
    // No factory voice data is required to create an empty library.
    for (auto& voice : voices)
    {
        std::fill(voice.begin() + 118, voice.end(), uint8_t(' '));
        constexpr char empty[] = "EMPTY";
        std::copy_n(empty, 5, voice.begin() + 118);
    }
}

bool VDX7UserBank::Snapshot::occupied(int slot) const noexcept
{
    return slot >= 0 && slot < 32 && (occupiedMask & (uint32_t(1) << slot)) != 0;
}

juce::String VDX7UserBank::Snapshot::name(int slot) const
{
    if (slot < 0 || slot >= 32) return {};
    return juce::String::fromUTF8(reinterpret_cast<const char*>(voices[slot].data() + 118), 10).trimEnd();
}

juce::Result VDX7UserBank::load(const juce::File& file, Snapshot& destination)
{
    if (file == juce::File() || file.isSymbolicLink()) return fail("Invalid USER bank path.");
    if (!file.exists())
    {
        destination = Snapshot {};
        return juce::Result::ok();
    }
    if (!file.existsAsFile() || file.getSize() != int64_t(fileSize))
        return fail("Invalid USER bank file size. The existing file was not changed.");
    juce::MemoryBlock data;
    if (!file.loadFileAsData(data) || data.getSize() != fileSize)
        return fail("Cannot read USER bank.");
    const auto* bytes = static_cast<const uint8_t*>(data.getData());
    if (std::memcmp(bytes, "VUB1", 4) != 0 || read32(bytes + payloadSize) != checksum(bytes, payloadSize))
        return fail("Unsupported or damaged USER bank. The existing file was not changed.");
    if (std::any_of(bytes + 8, bytes + payloadSize, [](uint8_t v) { return v > 127; }))
        return fail("USER bank contains invalid voice bytes.");
    Snapshot result;
    result.occupiedMask = read32(bytes + 4);
    result.exists = true;
    for (int slot = 0; slot < 32; ++slot)
        std::copy_n(bytes + 8 + slot * 128, 128, result.voices[slot].begin());
    destination = std::move(result);
    return juce::Result::ok();
}

juce::Result VDX7UserBank::savePatch(const juce::File& file, const Snapshot& expected,
                                   int slot, Voice patch, const juce::String& name,
                                   bool overwriteConfirmed)
{
    if (slot < 0 || slot >= 32) return fail("Choose a USER bank slot from 01 to 32.");
    if (name.isEmpty() || name.length() > 10 || name.trim().isEmpty())
        return fail("Use 1-10 printable ASCII characters for the patch name.");
    for (auto c : name)
        if (c < 32 || c > 126) return fail("Use 1-10 printable ASCII characters for the patch name.");
    if (std::any_of(patch.begin(), patch.end(), [](uint8_t v) { return v > 127; }))
        return fail("Invalid seven-bit voice data.");
    if (file == juce::File() || !file.getParentDirectory().isDirectory() || file.isSymbolicLink())
        return fail("USER bank folder is unavailable.");

    std::unique_lock<std::mutex> threadLock(localWriterMutex, std::try_to_lock);
    if (!threadLock.owns_lock()) return fail("USER bank is busy. Please try again.");
    juce::InterProcessLock processLock(lockName);
    if (!processLock.enter(0)) return fail("USER bank is busy in another process. Please try again.");
    struct Unlock { juce::InterProcessLock& lock; ~Unlock() { lock.exit(); } } unlock { processLock };

    Snapshot current;
    if (auto result = load(file, current); result.failed()) return result;
    if ((expected.exists && !current.exists)
        || current.occupied(slot) != expected.occupied(slot)
        || current.voices[slot] != expected.voices[slot])
        return fail("The target slot changed. Reload the bank and confirm the destination again.");
    if (current.occupied(slot) && !overwriteConfirmed)
        return fail("This USER slot is occupied. Confirm overwrite or choose an empty slot.");

    std::fill(patch.begin() + 118, patch.end(), uint8_t(' '));
    for (int i = 0; i < name.length(); ++i) patch[118 + i] = static_cast<uint8_t>(name[i]);
    current.voices[slot] = patch;
    current.occupiedMask |= uint32_t(1) << slot;
    std::array<uint8_t, fileSize> bytes {};
    std::memcpy(bytes.data(), "VUB1", 4);
    write32(bytes.data() + 4, current.occupiedMask);
    for (int i = 0; i < 32; ++i)
        std::copy(current.voices[i].begin(), current.voices[i].end(), bytes.begin() + 8 + i * 128);
    write32(bytes.data() + payloadSize, checksum(bytes.data(), payloadSize));

    // Sibling temporary file: the destination is untouched until replacement.
    juce::TemporaryFile temp(file);
    {
        juce::FileOutputStream stream(temp.getFile());
        if (!stream.openedOk() || !stream.write(bytes.data(), bytes.size()))
            return fail("Cannot write USER bank. Previous bank was kept.");
        stream.flush();
        if (stream.getStatus().failed()) return fail("Cannot flush USER bank. Previous bank was kept.");
    }
    Snapshot verified;
    if (load(temp.getFile(), verified).failed() || verified.voices != current.voices
        || verified.occupiedMask != current.occupiedMask)
        return fail("USER bank verification failed. Previous bank was kept.");
    if (!temp.overwriteTargetFileWithTemporary())
        return fail("Cannot replace USER bank. Check folder permissions.");
    return juce::Result::ok();
}
