#include "VDX7FactoryBanks.h"
#include "VDX7BoundedFile.h"
#include "VDX7Sysex.h"
#include "VDX7VoiceData.h"
#include "VDX7StateBytes.h"
#include <juce_cryptography/juce_cryptography.h>

namespace VDX7FactoryBanks
{
const ReferenceHashes& references()
{
    // Fingerprints only. No Yamaha voice bytes or firmware are distributed.
    // These identify reference dumps, not their copyright/provenance.
    static const ReferenceHashes hashes {{
        "687fb1153304ef7333420ba69fea6379cb064a6fc02c274c91c1023e3af80981",
        "436f450cd137455a2efb6e6be25f54709499e18cae281cfe269a22f5c4110dc8",
        "f54e7813ae56ca31b71c635851063614d0f37d66a99ce5770302b1aab3fba60d",
        "d58ec65822e593ee86cffbc432df89515c3c6eb3ccc1690fec1429ae76db8b29",
        "221f3fc3ea66358cbe4a04a23247f1948a4b3359dfc2d60bab45e3401b3f3568",
        "00a842f182dcda9e01f087cb27f9c0f541f49485dbd8b042ce21269ae8e0fd87",
        "3908263b180dee24053b541d891388d443381b6604c0dbd42abc3e5b894bef63",
        "8fc12b413bbf7b5d38be7da85a7eb882249846958271935c6dba6035670f8e04"
    }};
    return hashes;
}

juce::File defaultFolder()
{
    auto folder = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
#if JUCE_MAC
    folder = folder.getChildFile("Application Support");
#endif
    return folder.getChildFile("VDX7-JUCE").getChildFile("Factory Banks");
}

int identify(const std::vector<uint8_t>& packed, const ReferenceHashes& hashes)
{
    if (packed.size() != bankSize || !VDX7VoiceData::hasValidPackedVoices(packed.data(), packed.size()))
        return -1;
    const auto hash = juce::SHA256(packed.data(), packed.size()).toHexString();
    for (int i = 0; i < 8; ++i)
        if (hash == hashes[static_cast<std::size_t>(i)]) return i;
    return -1;
}

ScanResult scan(const juce::File& folder, const ReferenceHashes& hashes)
{
    ScanResult result;
    if (!folder.exists()) return result; // No library is a supported first run.
    if (!folder.isDirectory())
    {
        result.complete = false;
        result.warnings.add("Factory bank path is not a folder.");
        return result;
    }
    int count = 0;
    // Non-recursive enumeration; stop before unbounded collection/allocation.
    for (const auto& entry : juce::RangedDirectoryIterator(folder, false, "*", juce::File::findFiles))
    {
        const auto file = entry.getFile();
        if (!file.hasFileExtension("syx")) continue;
        if (++count > maxFiles)
        {
            result.complete = false;
            result.warnings.add("Only the first 128 SysEx files are scanned. Reduce the folder contents.");
            break;
        }
        std::vector<uint8_t> message, packed;
        if (!VDX7BoundedFile::read(file, VDX7Sysex::kBankMessageSize, message)
            || message.size() != VDX7Sysex::kBankMessageSize || !VDX7Sysex::decode(message, packed))
        {
            result.warnings.add(file.getFileName() + ": invalid 32-voice SysEx bank.");
            continue;
        }
        const int slot = identify(packed, hashes);
        if (slot < 0)
        {
            result.warnings.add(file.getFileName() + ": not a recognised factory bank; use LOAD SYX for CUSTOM.");
            continue;
        }
        if (result.banks.image.empty()) result.banks.image.resize(imageSize, 0);
        std::copy(packed.begin(), packed.end(), result.banks.image.begin() + slot * bankSize);
        result.banks.mask |= static_cast<uint8_t>(1u << slot);
    }
    return result;
}

bool valid(const Snapshot& banks)
{
    if (banks.mask == 0) return banks.image.empty();
    if (banks.image.size() != imageSize) return false;
    for (int slot = 0; slot < 8; ++slot)
    {
        const auto* first = banks.image.data() + slot * bankSize;
        if ((banks.mask & (1u << slot)) != 0)
        {
            if (!VDX7VoiceData::hasValidPackedVoices(first, bankSize)) return false;
        }
        else if (std::any_of(first, first + bankSize, [](uint8_t value) { return value != 0; }))
            return false;
    }
    return true;
}

void writeState(juce::ValueTree& state, const Snapshot& banks)
{
    jassert(valid(banks));
    state.setProperty("factoryBankMask", int(banks.mask), nullptr);
    state.setProperty("factoryBanks", juce::MemoryBlock(banks.image.data(), banks.image.size()).toBase64Encoding(), nullptr);
}

bool readState(const juce::ValueTree& state, Snapshot& banks, bool& present)
{
    present = state.hasProperty("factoryBankMask") || state.hasProperty("factoryBanks");
    if (!present) { banks = {}; return true; }
    if (!state.hasProperty("factoryBankMask") || !state.hasProperty("factoryBanks")) return false;
    const auto mask = state["factoryBankMask"].toString();
    if (mask.isEmpty() || mask.length() > 3 || !mask.containsOnly("0123456789") || mask.getIntValue() > 255)
        return false;
    const auto text = state["factoryBanks"].toString();
    juce::MemoryBlock block;
    if (!VDX7StateBytes::decode(text, mask.getIntValue() == 0 ? 0 : imageSize, block)) return false;
    Snapshot candidate;
    candidate.mask = static_cast<uint8_t>(mask.getIntValue());
    if (block.getSize() != 0)
    {
        const auto* bytes = static_cast<const uint8_t*>(block.getData());
        candidate.image.assign(bytes, bytes + block.getSize());
    }
    if (!valid(candidate)) return false;
    banks = std::move(candidate);
    return true;
}
}
