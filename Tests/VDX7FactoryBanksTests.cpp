#include "VDX7FactoryBanks.h"
#include "VDX7Sysex.h"
#include "VDX7MidiValidation.h"
#include <juce_cryptography/juce_cryptography.h>
#include <iostream>
#include <stdexcept>

static void require(bool ok, const char* text)
{ if (!ok) throw std::runtime_error(text); }

int main()
{
    const auto folder = juce::File::getSpecialLocation(juce::File::tempDirectory)
        .getNonexistentChildFile("vdx7-bank-scan", {}, false);
    try
    {
        require(folder.createDirectory().wasOk(), "create isolated synthetic folder");
        VDX7FactoryBanks::ReferenceHashes hashes;
        std::array<std::vector<uint8_t>, 8> banks;
        for (int slot = 0; slot < 8; ++slot)
        {
            banks[slot].resize(4096, 0);
            banks[slot][14] = static_cast<uint8_t>(slot + 1); // synthetic level, no Yamaha data
            hashes[slot] = juce::SHA256(banks[slot].data(), banks[slot].size()).toHexString();
            require(VDX7FactoryBanks::identify(banks[slot], hashes) == slot, "full-content identity");
            require(VDX7FactoryBanks::identify(banks[slot]) == -1, "synthetic hashes not in production table");
        }
        auto put = [&](const juce::String& name, const std::vector<uint8_t>& message) {
            require(folder.getChildFile(name).replaceWithData(message.data(), message.size()), "write synthetic fixture");
        };
        put("renamed.syx", VDX7Sysex::encode(banks[5]));
        put("rom1a.syx", VDX7Sysex::encode(banks[2])); // deliberately misleading name
        put("duplicate.SYX", VDX7Sysex::encode(banks[2]));
        auto unknown = banks[0]; unknown[14] = 50;
        put("rom4b.syx", VDX7Sysex::encode(unknown));
        auto invalid = VDX7Sysex::encode(banks[1]); invalid[100] ^= 1;
        put("checksum.syx", invalid);
        auto shortFile = invalid; shortFile.pop_back(); put("short.syx", shortFile);
        put("oversized.syx", std::vector<uint8_t>(100000, 0));
        put("single.syx", VDX7Sysex::encode(std::vector<uint8_t>(128, 0)));
        require(folder.getChildFile("nested").createDirectory().wasOk(), "nested directory");
        const auto nestedMessage = VDX7Sysex::encode(banks[7]);
        require(folder.getChildFile("nested/rom4b.syx").replaceWithData(nestedMessage.data(), nestedMessage.size()), "nested fixture");
        const auto partial = VDX7FactoryBanks::scan(folder, hashes);
        require(partial.complete && partial.banks.mask == ((1u << 2) | (1u << 5)), "partial banks occupy content-defined slots only");
        require(partial.warnings.size() == 5, "unknown, checksum, short, large and single diagnostics");
        require(VDX7FactoryBanks::valid(partial.banks), "valid partial snapshot");
        for (int slot = 0; slot < 128; ++slot)
        {
            const uint8_t midi[] {0xb0, 32, static_cast<uint8_t>(slot)};
            require(VDX7MidiValidation::acceptsHostEventWithBankMask(midi, 3, 0, partial.banks.mask)
                == (slot == 2 || slot == 5), "missing bank rejected before deferred MIDI storage");
        }
        require(!VDX7MidiValidation::acceptsHostEventWithBankMask(nullptr, 3, 0, 255), "mask helper validates syntax first");
        for (int slot : {2,5})
            require(std::equal(banks[slot].begin(), banks[slot].end(), partial.banks.image.begin()+slot*4096), "bytes unchanged");
        juce::MemoryBlock unchanged;
        require(folder.getChildFile("rom1a.syx").loadFileAsData(unchanged)
            && unchanged == juce::MemoryBlock(VDX7Sysex::encode(banks[2]).data(), 4104), "scanner never rewrites source");
        auto channel = VDX7Sysex::encode(banks[0]); channel[2] = 0x0f;
        put("channel.syx", channel);
        require(VDX7FactoryBanks::scan(folder, hashes).banks.mask == (partial.banks.mask | 1), "MIDI channel independent");

        juce::ValueTree state("VDX7STATE");
        VDX7FactoryBanks::writeState(state, partial.banks);
        const auto tree = juce::ValueTree::fromXml(*state.createXml());
        VDX7FactoryBanks::Snapshot restored;
        bool present = false;
        require(VDX7FactoryBanks::readState(tree, restored, present) && present
            && restored.mask == partial.banks.mask && restored.image == partial.banks.image, "project XML round trip");
        const auto good = restored;
        for (int probe = 0; probe < 7; ++probe)
        {
            auto bad = tree.createCopy();
            if (probe == 0) bad.removeProperty("factoryBanks", nullptr);
            if (probe == 1) bad.setProperty("factoryBankMask", -1, nullptr);
            if (probe == 2) bad.setProperty("factoryBankMask", 256, nullptr);
            if (probe == 3) bad.setProperty("factoryBanks", "not base64", nullptr);
            if (probe >= 4)
            {
                auto bytes = good.image;
                if (probe == 4) bytes.pop_back();
                if (probe == 5) bytes[2*4096+14] = 100; // invalid occupied voice
                if (probe == 6) bytes[0] = 1; // nonzero absent slot
                bad.setProperty("factoryBanks", juce::MemoryBlock(bytes.data(), bytes.size()).toBase64Encoding(), nullptr);
            }
            require(!VDX7FactoryBanks::readState(bad, restored, present)
                && restored.image == good.image && restored.mask == good.mask, "malformed snapshot rejected transactionally");
        }
        VDX7FactoryBanks::writeState(state, {});
        require(VDX7FactoryBanks::readState(state, restored, present) && present && restored.mask == 0, "empty catalog saved explicitly");
        require(VDX7FactoryBanks::readState(juce::ValueTree("VDX7STATE"), restored, present) && !present, "legacy state remains supported");
        require(VDX7FactoryBanks::scan(folder.getChildFile("missing")).banks.mask == 0, "missing folder supported");
        require(!VDX7FactoryBanks::scan(folder.getChildFile("renamed.syx")).complete, "file cannot replace library directory");
        for (int slot = 0; slot < 8; ++slot) put(juce::String(slot)+".syx", VDX7Sysex::encode(banks[slot]));
        require(VDX7FactoryBanks::scan(folder, hashes).banks.mask == 255, "all eight slots supported");
        for (int i = 0; i < 130; ++i) put("duplicate-" + juce::String(i) + ".syx", VDX7Sysex::encode(banks[0]));
        const auto excessive = VDX7FactoryBanks::scan(folder, hashes);
        require(!excessive.complete && excessive.warnings.joinIntoString("\n").contains("128"), "bounded incomplete scan cannot replace live library");
        std::cout << "PASS: content identity, partial/all banks, unchanged sources, bounded validation and project snapshots\n";
        require(folder.deleteRecursively(), "clean isolated fixtures");
        return 0;
    }
    catch (const std::exception& error)
    {
        folder.deleteRecursively();
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
