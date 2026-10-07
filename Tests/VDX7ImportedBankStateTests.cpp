#include "VDX7ImportedBankState.h"
#include "VDX7InitVoice.h"
#include <juce_cryptography/juce_cryptography.h>
#include <iostream>
#include <stdexcept>

namespace
{
using namespace VDX7ImportedBanks;
void require(bool ok, const char* message)
{ if (!ok) throw std::runtime_error(message); }

Bank syntheticBank(int serial)
{
    auto voice = vdx7InitVoice();
    voice[0] = static_cast<uint8_t>(serial % 100);
    voice[1] = static_cast<uint8_t>(serial / 100);
    std::vector<uint8_t> packed;
    for (int i = 0; i < 32; ++i) packed.insert(packed.end(), voice.begin(), voice.end());
    const auto name = "long-shared-prefix-bank-" + juce::String(serial) + ".syx";
    return { juce::SHA256(packed.data(), packed.size()).toHexString(), name, displayName(name), std::move(packed) };
}

bool equal(const Snapshot& a, const Snapshot& b)
{
    if (a.selectedId != b.selectedId || a.banks.size() != b.banks.size()) return false;
    for (std::size_t i = 0; i < a.banks.size(); ++i)
        if (a.banks[i].contentId != b.banks[i].contentId || a.banks[i].fileName != b.banks[i].fileName
            || a.banks[i].displayName != b.banks[i].displayName || a.banks[i].packed != b.banks[i].packed) return false;
    return true;
}

juce::String encodedName(const juce::String& name)
{ return juce::MemoryBlock(name.toRawUTF8(), name.getNumBytesAsUTF8()).toBase64Encoding(); }
}

void testImportedBankState()
{
    using namespace VDX7ImportedBanks;
    Snapshot source;
    source.banks = {syntheticBank(0), syntheticBank(1)};
    source.banks[1].packed[0] = 127; // Preserve accepted legacy VMEM exactly.
    source.banks[1].packed[16] = 100;
    source.banks[1].contentId = juce::SHA256(source.banks[1].packed.data(), source.banks[1].packed.size()).toHexString();
    source.selectedId = source.banks[1].contentId;
    require(validSnapshot(source), "valid synthetic selected catalog");
    juce::ValueTree state("VDX7STATE");
    state.setProperty("ram", "untouched working patch", nullptr);
    state.addChild(juce::ValueTree("PARAMETERS"), -1, nullptr);
    require(writeState(state, source), "write prepared snapshot");
    require(state["ram"].toString() == "untouched working patch"
        && state.getChildWithName("PARAMETERS").isValid(), "unrelated patch/parameter state untouched");
    Snapshot restored;
    bool present = false;
    require(readState(state, restored, present) && present && equal(source, restored), "direct tree round trip");
    const auto xmlTree = juce::ValueTree::fromXml(*state.createXml());
    require(readState(xmlTree, restored, present) && equal(source, restored), "XML version strings and payload round trip");
    require(restored.banks[0].displayName == restored.banks[1].displayName
        && restored.banks[0].contentId != restored.banks[1].contentId, "label collision does not lose identity");

    auto renamed = xmlTree.createCopy();
    const auto unicodeName = juce::String::charToString(0x1f3b9) + "renamed\\bank.syx";
    renamed.getChildWithName("ImportedBanks").getChild(1).setProperty("fileName", encodedName(unicodeName), nullptr);
    require(readState(renamed, restored, present) && restored.selectedId == source.selectedId
        && restored.banks[1].packed == source.banks[1].packed && restored.banks[1].fileName == unicodeName,
        "opaque cross-platform filename changes never change saved sound/selection");
    require(readState(xmlTree, restored, present), "reset destination fixture");
    const auto good = restored;
    auto reject = [&](juce::ValueTree bad)
    {
        for (bool previousPresence : {false, true})
        {
            present = previousPresence;
            require(!readState(bad, restored, present) && equal(restored, good) && present == previousPresence,
                "invalid snapshot cannot partially replace data or presence");
        }
    };
    for (const juce::var& version : {juce::var("2"), juce::var("01"), juce::var("1junk"),
         juce::var(""), juce::var(true), juce::var(1.0)})
    {
        auto bad = state.createCopy();
        bad.getChildWithName("ImportedBanks").setProperty("version", version, nullptr);
        reject(bad);
    }
    for (int probe = 0; probe < 20; ++probe)
    {
        auto bad = state.createCopy();
        auto catalog = bad.getChildWithName("ImportedBanks"), entry = catalog.getChild(0);
        if (probe == 0) catalog.removeProperty("version", nullptr);
        if (probe == 1) catalog.removeProperty("selectedId", nullptr);
        if (probe == 2) catalog.setProperty("extra", "not version 1", nullptr);
        if (probe == 3) catalog.setProperty("selectedId", juce::String::repeatedString("0", 64), nullptr);
        if (probe == 4) catalog.setProperty("selectedId", 0, nullptr);
        if (probe == 5) entry.setProperty("id", source.banks[0].contentId.toUpperCase(), nullptr);
        if (probe == 6) entry.setProperty("id", source.banks[1].contentId, nullptr); // Wrong content hash.
        if (probe == 7) entry.removeProperty("data", nullptr);
        if (probe == 8) entry.setProperty("data", juce::MemoryBlock(4095, true).toBase64Encoding(), nullptr);
        if (probe == 9) entry.setProperty("data", "2147483647.", nullptr);
        if (probe == 10) entry.setProperty("data", entry["data"].toString() + ".", nullptr);
        if (probe == 11) entry.setProperty("fileName", encodedName(juce::String::repeatedString("x", 1025) + ".syx"), nullptr);
        if (probe == 12) entry.setProperty("fileName", encodedName("../bank.syx"), nullptr);
        if (probe == 13) entry.setProperty("fileName", encodedName("voice.txt"), nullptr);
        if (probe == 14) entry.addChild(juce::ValueTree("Nested"), -1, nullptr);
        if (probe == 15) catalog.addChild(entry.createCopy(), -1, nullptr);
        if (probe == 16) bad.addChild(catalog.createCopy(), -1, nullptr);
        if (probe == 17) entry.setProperty("id", juce::String::repeatedString("0", 64), nullptr);
        if (probe == 18) entry.setProperty("extra", "unknown field", nullptr);
        if (probe == 19) entry.setProperty("data", entry["data"].toString().replaceSection(7, 1, "!"), nullptr);
        reject(bad);
    }
    for (const auto& property : {"id", "fileName", "data"})
    {
        auto bad = state.createCopy();
        bad.getChildWithName("ImportedBanks").getChild(0).setProperty(property, 123, nullptr);
        reject(bad);
    }
    auto invalidVoice = state.createCopy();
    auto bytes = source.banks[0].packed;
    bytes[14] = 100; // Semantically invalid but hash-consistent input.
    auto entry = invalidVoice.getChildWithName("ImportedBanks").getChild(0);
    entry.setProperty("data", juce::MemoryBlock(bytes.data(), bytes.size()).toBase64Encoding(), nullptr);
    entry.setProperty("id", juce::SHA256(bytes.data(), bytes.size()).toHexString(), nullptr);
    reject(invalidVoice);
    auto unknownEntry = state.createCopy();
    unknownEntry.getChildWithName("ImportedBanks").addChild(juce::ValueTree("NotABank"), -1, nullptr);
    reject(unknownEntry);
    reject(juce::ValueTree());
    const uint8_t invalidUtf8[] {0xff, '.', 's', 'y', 'x'};
    const uint8_t embeddedNull[] {'x', 0, '.', 's', 'y', 'x'};
    for (const auto& text : {juce::String("2147483647."), juce::String("0005.ABC"),
         juce::MemoryBlock(invalidUtf8, sizeof(invalidUtf8)).toBase64Encoding(),
         juce::MemoryBlock(embeddedNull, sizeof(embeddedNull)).toBase64Encoding()})
    {
        auto bad = state.createCopy();
        bad.getChildWithName("ImportedBanks").getChild(0).setProperty("fileName", text, nullptr);
        reject(bad);
    }

    const auto unchanged = state.createCopy();
    for (int probe = 0; probe < 4; ++probe)
    {
        auto bad = source;
        if (probe == 0) bad.selectedId = juce::String::repeatedString("0", 64);
        if (probe == 1) bad.banks.push_back(bad.banks.front());
        if (probe == 2) bad.banks[0].packed.pop_back();
        if (probe == 3) bad.banks[0].fileName = "";
        require(!writeState(state, bad) && state.isEquivalentTo(unchanged), "invalid save leaves whole root untouched");
    }
    Snapshot maximum;
    for (int i = 0; i < maxFiles; ++i) maximum.banks.push_back(syntheticBank(i));
    maximum.selectedId = maximum.banks.back().contentId;
    require(writeState(state, maximum) && readState(juce::ValueTree::fromXml(*state.createXml()), restored, present)
        && equal(maximum, restored), "128-bank 512 KiB XML round trip");
    auto oversized = state.createCopy();
    auto catalog = oversized.getChildWithName("ImportedBanks");
    catalog.addChild(catalog.getChild(0).createCopy(), -1, nullptr);
    // Rejection must preserve this maximum-sized destination too.
    const auto maximumGood = restored;
    require(!readState(oversized, restored, present) && equal(maximumGood, restored), "129-entry read rejected transactionally");
    maximum.banks.push_back(syntheticBank(128));
    const auto beforeOverflow = state.createCopy();
    require(!writeState(state, maximum) && state.isEquivalentTo(beforeOverflow), "129-entry save rejected transactionally");
    auto boundary = source;
    boundary.banks[0].fileName = juce::String::repeatedString(juce::String::charToString(0x1f3b9), 255) + ".syx";
    require(boundary.banks[0].fileName.getNumBytesAsUTF8() == maxFileNameBytes
        && writeState(state, boundary) && readState(state, restored, present)
        && restored.banks[0].fileName == boundary.banks[0].fileName, "1024-byte Unicode metadata boundary");
    boundary.banks[0].fileName = "x" + boundary.banks[0].fileName;
    require(!validSnapshot(boundary), "UTF-8 bytes, not code points, bound metadata");
    auto misleadingLabel = source;
    misleadingLabel.banks[0].displayName = "do not persist this";
    require(writeState(state, misleadingLabel) && readState(state, restored, present) && equal(restored, source),
        "display labels are derived, not trusted serialized identity");
    auto controlName = source;
    controlName.banks[0].fileName = "literal" + juce::String::charToString(1) + "\n&<bank.syx";
    require(writeState(state, controlName)
        && readState(juce::ValueTree::fromXml(*state.createXml()), restored, present)
        && restored.banks[0].fileName == controlName.banks[0].fileName
        && !restored.banks[0].displayName.containsChar('\n'), "XML-safe metadata preserves legal POSIX control filenames");
    require(writeState(state, {}) && readState(state, restored, present) && present && restored.banks.empty()
        && restored.selectedId.isEmpty() && state.getNumChildren() == 2, "explicit empty catalog replaces previous snapshot only");
    restored = source;
    require(readState(juce::ValueTree("VDX7STATE"), restored, present) && !present && restored.banks.empty(),
        "legacy absence clears catalog without fabricating selection");
    std::cout << "PASS: imported catalog state XML, identity/legacy preservation, bounds and transactional rejection\n";
}
