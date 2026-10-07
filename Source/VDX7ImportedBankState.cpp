#include "VDX7ImportedBankState.h"
#include "VDX7StateBytes.h"
#include "VDX7VoiceData.h"
#include <juce_cryptography/juce_cryptography.h>
#include <algorithm>

namespace VDX7ImportedBanks
{
static bool validId(const juce::String& id)
{
    return id.length() == 64 && id.containsOnly("0123456789abcdef");
}

static bool validFileName(const juce::String& name)
{
    // Metadata only: never concatenate this with a folder or open it on recall.
    // Backslashes are legal macOS filenames and must survive cross-platform recall.
    return name.isNotEmpty() && name.getNumBytesAsUTF8() <= maxFileNameBytes
        && !name.containsChar('/') && name.endsWithIgnoreCase(".syx");
}

static juce::String encodeFileName(const juce::String& name)
{
    // Encode metadata too: POSIX filenames may contain XML-illegal control
    // characters. Preserve the filename, rather than editing the source/label.
    return juce::MemoryBlock(name.toRawUTF8(), name.getNumBytesAsUTF8()).toBase64Encoding();
}

static bool decodeFileName(const juce::String& text, juce::String& name)
{
    if (text.length() > 5 + (maxFileNameBytes * 8 + 5) / 6) return false;
    const int dot = text.indexOfChar('.');
    if (dot < 1 || dot > 4) return false;
    const auto prefix = text.substring(0, dot);
    if (!prefix.containsOnly("0123456789")) return false;
    const int size = prefix.getIntValue();
    if (size < 1 || size > maxFileNameBytes) return false;
    juce::MemoryBlock bytes;
    if (!VDX7StateBytes::decode(text, static_cast<std::size_t>(size), bytes)) return false;
    const auto* utf8 = static_cast<const char*>(bytes.getData());
    if (!juce::CharPointer_UTF8::isValidString(utf8, size)) return false;
    const auto candidate = juce::String::fromUTF8(utf8, size);
    if (!validFileName(candidate) || encodeFileName(candidate) != text) return false;
    name = candidate;
    return true;
}

bool validSnapshot(const Snapshot& snapshot)
{
    if (snapshot.banks.size() > maxFiles
        || (snapshot.selectedId.isNotEmpty() && !validId(snapshot.selectedId))) return false;
    bool foundSelection = snapshot.selectedId.isEmpty();
    for (std::size_t index = 0; index < snapshot.banks.size(); ++index)
    {
        const auto& bank = snapshot.banks[index];
        if (!validId(bank.contentId) || !validFileName(bank.fileName)
            || bank.packed.size() != VDX7FactoryBanks::bankSize
            || !VDX7VoiceData::hasValidPackedVoices(bank.packed.data(), bank.packed.size())
            || juce::SHA256(bank.packed.data(), bank.packed.size()).toHexString() != bank.contentId
            || VDX7FactoryBanks::identify(bank.packed) >= 0) return false;
        for (std::size_t earlier = 0; earlier < index; ++earlier)
            if (snapshot.banks[earlier].contentId == bank.contentId) return false;
        if (bank.contentId == snapshot.selectedId) foundSelection = true;
    }
    return foundSelection;
}

bool writeState(juce::ValueTree& state, const Snapshot& snapshot)
{
    if (!state.isValid() || !validSnapshot(snapshot)) return false;
    juce::ValueTree catalog("ImportedBanks");
    catalog.setProperty("version", 1, nullptr);
    catalog.setProperty("selectedId", snapshot.selectedId, nullptr);
    for (const auto& bank : snapshot.banks)
    {
        juce::ValueTree entry("Bank");
        entry.setProperty("id", bank.contentId, nullptr);
        entry.setProperty("fileName", encodeFileName(bank.fileName), nullptr);
        entry.setProperty("data", juce::MemoryBlock(bank.packed.data(), bank.packed.size()).toBase64Encoding(), nullptr);
        catalog.addChild(entry, -1, nullptr);
    }
    // Build/validate before mutating the owned tree. Other state stays intact.
    for (int index = state.getNumChildren() - 1; index >= 0; --index)
        if (state.getChild(index).hasType("ImportedBanks")) state.removeChild(index, nullptr);
    state.addChild(catalog, -1, nullptr);
    return true;
}

bool readState(const juce::ValueTree& state, Snapshot& snapshot, bool& present)
{
    if (!state.isValid()) return false;
    juce::ValueTree catalog;
    for (const auto& child : state)
        if (child.hasType("ImportedBanks"))
        {
            if (catalog.isValid()) return false; // Ambiguous duplicate subtrees.
            catalog = child;
        }
    if (!catalog.isValid()) { snapshot = {}; present = false; return true; }
    const auto version = catalog["version"];
    const auto selection = catalog["selectedId"];
    // XML turns numeric attributes into strings; do not accept prefix coercion.
    if (catalog.getNumProperties() != 2 || !catalog.hasProperty("version")
        || !catalog.hasProperty("selectedId")
        || (!version.isInt() && !version.isInt64() && !version.isString())
        || version.toString() != "1" || !selection.isString()
        || catalog.getNumChildren() > maxFiles) return false;
    Snapshot candidate;
    candidate.selectedId = selection.toString();
    if (candidate.selectedId.isNotEmpty() && !validId(candidate.selectedId)) return false;
    candidate.banks.reserve(static_cast<std::size_t>(catalog.getNumChildren()));
    for (const auto& entry : catalog)
    {
        const auto id = entry["id"], name = entry["fileName"], data = entry["data"];
        if (!entry.hasType("Bank") || entry.getNumChildren() != 0 || entry.getNumProperties() != 3
            || !id.isString() || !name.isString() || !data.isString()
            || !validId(id.toString())) return false;
        juce::String decodedName;
        if (!decodeFileName(name.toString(), decodedName)) return false;
        juce::MemoryBlock packed;
        if (!VDX7StateBytes::decode(data.toString(), VDX7FactoryBanks::bankSize, packed)) return false;
        const auto* first = static_cast<const uint8_t*>(packed.getData());
        candidate.banks.push_back({ id.toString(), decodedName, displayName(decodedName),
            std::vector<uint8_t>(first, first + packed.getSize()) });
    }
    if (!validSnapshot(candidate)) return false;
    snapshot = std::move(candidate);
    present = true;
    return true;
}
}
