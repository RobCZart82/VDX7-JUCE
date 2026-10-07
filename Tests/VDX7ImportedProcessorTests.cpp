#include "PluginProcessor.h"
#include "VDX7InitVoice.h"
#include "VDX7Sysex.h"
#include <juce_cryptography/juce_cryptography.h>
#include <functional>
#include <iostream>
#include <stdexcept>

static std::function<void()> scanBoundary, saveBoundary;
void vdx7TestImportedBankScanBoundary()
{ auto action = std::move(scanBoundary); scanBoundary = {}; if (action) action(); }
void vdx7TestRomStateBoundary()
{ auto action = std::move(saveBoundary); saveBoundary = {}; if (action) action(); }
void vdx7TestStateBoundary(std::size_t) {}
static void require(bool ok, const char* message)
{ if (!ok) throw std::runtime_error(message); }

struct Fixtures
{
    juce::File root = juce::File::getSpecialLocation(juce::File::tempDirectory)
        .getNonexistentChildFile("vdx7-imported-processor", {}, false);
    Fixtures() { require(root.createDirectory().wasOk(), "isolated test folder"); }
    ~Fixtures() { root.deleteRecursively(); }
    juce::File folder(const char* name)
    { auto result = root.getChildFile(name); require(result.createDirectory().wasOk(), "synthetic subfolder"); return result; }
};

static VDX7ImportedBanks::Bank syntheticBank(int serial)
{
    auto seed = vdx7InitVoice();
    seed[0] = static_cast<uint8_t>(serial % 100);
    seed[1] = static_cast<uint8_t>(serial / 100);
    std::vector<uint8_t> packed;
    for (int slot = 0; slot < 32; ++slot) packed.insert(packed.end(), seed.begin(), seed.end());
    const auto name = "bank-" + juce::String(serial) + ".syx";
    return {juce::SHA256(packed.data(), packed.size()).toHexString(), name,
            VDX7ImportedBanks::displayName(name), std::move(packed)};
}
static void put(const juce::File& folder, const VDX7ImportedBanks::Bank& bank)
{
    const auto message = VDX7Sysex::encode(bank.packed);
    require(folder.getChildFile(bank.fileName).replaceWithData(message.data(), message.size()), "write own synthetic bank");
}
static juce::MemoryBlock save(VDX7AudioProcessor& p)
{ juce::MemoryBlock data; p.getStateInformation(data); return data; }
static juce::ValueTree tree(const juce::MemoryBlock& data)
{
    auto xml = juce::AudioProcessor::getXmlFromBinary(data.getData(), int(data.getSize()));
    require(xml != nullptr, "saved binary XML");
    return juce::ValueTree::fromXml(*xml);
}
static juce::MemoryBlock binary(const juce::ValueTree& state)
{ juce::MemoryBlock result; juce::AudioProcessor::copyXmlToBinary(*state.createXml(), result); return result; }
static void restore(VDX7AudioProcessor& p, const juce::MemoryBlock& data)
{ p.setStateInformation(data.getData(), int(data.getSize())); }
static VDX7ImportedBanks::Snapshot catalog(VDX7AudioProcessor& p)
{
    VDX7ImportedBanks::Snapshot result;
    bool present = false;
    require(VDX7ImportedBanks::readState(tree(save(p)), result, present) && present, "catalog saved in actual processor state");
    return result;
}
static void setFeedback(VDX7AudioProcessor& p, int value)
{
    auto* parameter = p.parameters().getParameter(VDX7ParameterIDs::voiceParameter(VDX7VoiceData::VoiceParameter::feedback));
    parameter->setValueNotifyingHost(parameter->convertTo0to1(float(value)));
}

struct VDX7RegressionAccess
{
    static bool pendingOwnsCatalog(VDX7AudioProcessor& p)
    {
        std::scoped_lock lock(p.engineMutex_);
        return p.pendingRestore_.getChildWithName("ImportedBanks").isValid();
    }
    static std::vector<uint8_t> ram(VDX7AudioProcessor& p)
    {
        std::scoped_lock lock(p.engineMutex_);
        std::vector<uint8_t> data;
        require(p.engine_.saveRam(data), "capture actual engine RAM");
        return data;
    }
};

static void testNoRom()
{
    Fixtures fixtures;
    const auto folder = fixtures.folder("library");
    const auto first = syntheticBank(200), second = syntheticBank(201);
    put(folder, first); put(folder, second);
    auto p = std::make_unique<VDX7AudioProcessor>(false);
    require(!p->getImportedBankSnapshot() && !tree(save(*p)).getChildWithName("ImportedBanks").isValid(),
        "fresh instance does not scan/create an owner library");
    juce::String report;
    require(p->refreshImportedBanks(folder, report), "explicit pre-ROM catalog refresh");
    const auto before = p->getImportedBankSnapshot();
    require(before && before->banks.size() == 2 && before->selectedId.isEmpty(), "immutable catalog has no fabricated live origin");
    setFeedback(*p, 6);
    const auto saved = save(*p);
    require(tree(saved).getChildWithName("DeferredVoiceEdits")["voice9"].toString() == "6", "catalog save preserves queued voice edits");
    require(!p->refreshImportedBanks(folder, report, [] { return true; }) && p->getImportedBankSnapshot() == before,
        "cancelled scan cannot publish partial/empty catalog");
    int queries = 0;
    require(p->refreshImportedBanks(folder, report, [&] { ++queries; return false; }), "count full refresh boundaries");
    const int finalQuery = queries;
    queries = 0;
    const auto stable = p->getImportedBankSnapshot();
    require(!p->refreshImportedBanks(folder, report, [&] { return ++queries == finalQuery; })
        && p->getImportedBankSnapshot() == stable, "post-validation cancellation cannot publish");
    require(!p->refreshImportedBanks(folder.getChildFile(first.fileName), report)
        && p->getImportedBankSnapshot() == stable, "path failure preserves catalog");
    const auto excessive = fixtures.folder("excessive");
    for (int i = 0; i < 129; ++i)
    {
        auto alias = first; alias.fileName = juce::String(i) + ".syx"; put(excessive, alias);
    }
    require(!p->refreshImportedBanks(excessive, report) && p->getImportedBankSnapshot() == stable, "limit failure preserves catalog");
    auto q = std::make_unique<VDX7AudioProcessor>(false);
    restore(*q, saved);
    require(!VDX7RegressionAccess::pendingOwnsCatalog(*q)
        && q->getImportedBankSnapshot()->banks.size() == 2 && catalog(*q).banks[0].packed == first.packed,
        "pending project carries complete bank bytes without firmware");
    require(folder.deleteRecursively(), "remove only isolated synthetic source library");
    setFeedback(*q, 5);
    require(catalog(*q).banks[0].contentId == first.contentId
        && tree(save(*q)).getChildWithName("DeferredVoiceEdits")["voice9"].toString() == "5",
        "deleted source cannot change pending bank data or later edits");
    require(!q->refreshImportedBanks(folder, report), "pending project blocks refresh before matching ROM");
    const auto kept = q->getImportedBankSnapshot();
    auto bad = tree(saved);
    bad.getChildWithName("ImportedBanks").setProperty("version", 2, nullptr);
    restore(*q, binary(bad));
    require(q->getImportedBankSnapshot() == kept, "invalid version cannot replace live/pending state");
    bad = tree(saved);
    bad.getChildWithName("ImportedBanks").setProperty("selectedId", first.contentId, nullptr);
    restore(*q, binary(bad));
    require(q->getImportedBankSnapshot() == kept, "origin without working RAM rejected");
    bad.setProperty("ram", juce::MemoryBlock(6144, true).toBase64Encoding(), nullptr);
    bad.setProperty("bank", 0, nullptr);
    restore(*q, binary(bad));
    require(q->getImportedBankSnapshot() == kept, "imported origin cannot impersonate factory selection");
    juce::MemoryBlock tooLarge(VDX7AudioProcessor::maxProjectStateBytes + 1, true);
    restore(*q, tooLarge);
    q->setStateInformation(nullptr, VDX7AudioProcessor::maxProjectStateBytes);
    q->setStateInformation(saved.getData(), -1);
    require(q->getImportedBankSnapshot() == kept, "oversized/null/negative input rejected before parser or state mutation");
    VDX7ImportedBanks::Snapshot maximum;
    for (int i = 0; i < 128; ++i)
    {
        auto bank = syntheticBank(i);
        bank.fileName = juce::String::repeatedString(juce::String::charToString(0x1f3b9), 255) + ".syx";
        maximum.banks.push_back(std::move(bank));
    }
    auto fullTree = tree(saved);
    require(VDX7ImportedBanks::writeState(fullTree, maximum), "maximal own project fixture");
    const auto fullBinary = binary(fullTree);
    require(fullBinary.getSize() < VDX7AudioProcessor::maxProjectStateBytes, "2 MiB input cap accommodates maximal catalog plus processor state");
    restore(*q, fullBinary);
    require(q->getImportedBankSnapshot()->banks.size() == 128 && catalog(*q).banks.back().packed == maximum.banks.back().packed,
        "128-bank maximal Unicode actual processor binary round trip");
    auto exactBoundary = saved;
    exactBoundary.setSize(VDX7AudioProcessor::maxProjectStateBytes, true);
    restore(*q, exactBoundary);
    require(q->getImportedBankSnapshot()->banks.size() == 2, "exact binary input boundary still admitted");
    auto empty = tree(saved);
    require(VDX7ImportedBanks::writeState(empty, {}), "explicit empty fixture");
    restore(*q, binary(empty));
    require(q->getImportedBankSnapshot() && q->getImportedBankSnapshot()->banks.empty(), "explicit empty remains present");
    empty.removeChild(empty.getChildWithName("ImportedBanks"), nullptr);
    restore(*q, binary(empty));
    require(!q->getImportedBankSnapshot() && !tree(save(*q)).getChildWithName("ImportedBanks").isValid(), "legacy absence does not reuse prior catalog");
    require(before->banks[0].packed == first.packed && stable->banks.size() == 2, "old handles remain immutable and alive");
}

static void testBoundaries()
{
    Fixtures fixtures;
    const auto a = fixtures.folder("a"), b = fixtures.folder("b");
    put(a, syntheticBank(200)); put(b, syntheticBank(201));
    juce::String report;
    auto p = std::make_unique<VDX7AudioProcessor>(false);
    require(p->refreshImportedBanks(a, report), "initial boundary catalog");
    scanBoundary = [&] { require(p->refreshImportedBanks(b, report), "newer nested scan publication"); };
    require(!p->refreshImportedBanks(a, report) && catalog(*p).banks[0].contentId == syntheticBank(201).contentId,
        "older scan cannot overwrite a newer library generation");
    saveBoundary = [&] { require(p->refreshImportedBanks(a, report), "refresh after locked save capture"); };
    const auto detached = save(*p);
    VDX7ImportedBanks::Snapshot old;
    bool present = false;
    require(VDX7ImportedBanks::readState(tree(detached), old, present)
        && old.banks[0].contentId == syntheticBank(201).contentId
        && catalog(*p).banks[0].contentId == syntheticBank(200).contentId,
        "save serializes the detached capture, not a newer mutable catalog");
    const auto stateA = save(*p);
    auto other = std::make_unique<VDX7AudioProcessor>(false);
    require(other->refreshImportedBanks(b, report), "other instance catalog");
    const auto otherState = save(*other);
    scanBoundary = [&] { restore(*p, otherState); };
    require(!p->refreshImportedBanks(a, report) && catalog(*p).banks[0].contentId == syntheticBank(201).contentId,
        "project restore during scan invalidates stale publication");
    saveBoundary = [&] { restore(*p, stateA); };
    const auto pendingDetached = save(*p);
    VDX7ImportedBanks::Snapshot pendingOld;
    require(VDX7ImportedBanks::readState(tree(pendingDetached), pendingOld, present)
        && pendingOld.banks[0].contentId == syntheticBank(201).contentId
        && catalog(*p).banks[0].contentId == syntheticBank(200).contentId,
        "pending save attaches its own detached catalog, not a concurrently restored one");
    require(other->getImportedBankSnapshot()->banks[0].contentId == syntheticBank(201).contentId,
        "instances do not share mutable catalog/state");
}

static void testWithRom(const juce::File& rom)
{
    Fixtures fixtures;
    const auto folder = fixtures.folder("library");
    const auto selected = syntheticBank(200);
    put(folder, selected); put(folder, syntheticBank(201));
    auto p = std::make_unique<VDX7AudioProcessor>(false);
    p->prepareToPlay(48000, 128);
    require(p->loadRomFromFile(rom), "private verified firmware");
    juce::String report;
    require(p->refreshImportedBanks(folder, report), "loaded explicit catalog refresh");
    setFeedback(*p, 6);
    auto edited = tree(save(*p)); // Apply the queued edit before the preservation oracle.
    const auto before = VDX7RegressionAccess::ram(*p);
    require(p->refreshImportedBanks(folder, report) && VDX7RegressionAccess::ram(*p) == before
        && p->isCurrentVoiceModified(), "loaded refresh preserves all engine RAM and dirty working voice");
    VDX7ImportedBanks::Snapshot snapshot;
    bool present = false;
    require(VDX7ImportedBanks::readState(edited, snapshot, present), "read processor catalog");
    // Synthetic project-origin fixture only: no new imported bank selector here.
    snapshot.selectedId = selected.contentId;
    require(VDX7ImportedBanks::writeState(edited, snapshot), "attach validated project origin");
    edited.setProperty("bank", -1, nullptr);
    const auto project = binary(edited);
    restore(*p, project);
    require(p->isProjectReady() && p->isCurrentVoiceModified()
        && catalog(*p).selectedId == selected.contentId
        && std::equal(before.begin(), before.begin() + 4096, VDX7RegressionAccess::ram(*p).begin()),
        "restore uses edited working RAM, never the original imported bank bytes");
    const auto oldHandle = p->getImportedBankSnapshot();
    require(folder.getChildFile(selected.fileName).deleteFile(), "remove isolated selected source");
    const auto restoredRam = VDX7RegressionAccess::ram(*p);
    require(p->refreshImportedBanks(folder, report) && catalog(*p).selectedId == selected.contentId
        && p->getImportedBankSnapshot()->banks.size() == 2 && VDX7RegressionAccess::ram(*p) == restoredRam,
        "refresh retains missing selected project bank without altering sound");
    const auto excessive = fixtures.folder("full");
    for (int i = 0; i < 128; ++i) put(excessive, syntheticBank(i));
    const auto kept = p->getImportedBankSnapshot();
    require(!p->refreshImportedBanks(excessive, report) && p->getImportedBankSnapshot() == kept,
        "full catalog cannot discard selected missing project bank to make room");
    require(folder.deleteRecursively(), "remove isolated remaining files");
    auto reopened = std::make_unique<VDX7AudioProcessor>(false);
    restore(*reopened, project); // Saved ROM path is permitted; bank folder is gone.
    require(reopened->isProjectReady() && reopened->isCurrentVoiceModified()
        && catalog(*reopened).selectedId == selected.contentId
        && std::equal(before.begin(), before.begin() + 4096, VDX7RegressionAccess::ram(*reopened).begin()),
        "actual processor binary recall is independent of deleted source bank folder");
    juce::AudioBuffer<float> audio(2, 128); juce::MidiBuffer midi;
    auto unavailable = edited.createCopy();
    unavailable.setProperty("romPath", fixtures.root.getChildFile("missing-firmware.bin").getFullPathName(), nullptr);
    auto waiting = std::make_unique<VDX7AudioProcessor>(false);
    restore(*waiting, binary(unavailable));
    require(!waiting->isProjectReady() && catalog(*waiting).selectedId == selected.contentId,
        "missing firmware preserves imported origin and working RAM as pending project");
    setFeedback(*waiting, 5);
    require(waiting->loadRomFromFile(rom) && waiting->isProjectReady()
        && catalog(*waiting).selectedId == selected.contentId
        && waiting->isCurrentVoiceModified()
        && tree(save(*waiting))["bank"].toString() == "-1",
        "matching later firmware installs pending project without discarding imported origin or later edits");
    const auto restoredFeedback = waiting->parameters().getParameter(
        VDX7ParameterIDs::voiceParameter(VDX7VoiceData::VoiceParameter::feedback));
    require(juce::roundToInt(restoredFeedback->convertFrom0to1(restoredFeedback->getValue())) == 5,
        "post-restore edit wins after delayed firmware installation");
    scanBoundary = [&] {
        require(p->selectFactoryBank(0), "queued existing factory bank selection");
        p->processBlock(audio, midi);
    };
    require(!p->refreshImportedBanks(folder, report) && p->getImportedBankSnapshot() == kept
        && report.contains("changed during scan"),
        "audio bank-origin transition invalidates older scan publication");
    require(catalog(*p).selectedId.isEmpty(), "audio-owned factory selection clears imported origin without catalog mutation");
    restore(*p, project);
    const auto single = VDX7Sysex::encode(std::vector<uint8_t>(selected.packed.begin(), selected.packed.begin() + 128));
    const auto singleFile = fixtures.root.getChildFile("single.syx");
    require(singleFile.replaceWithData(single.data(), single.size()) && p->loadSyxFromFile(singleFile)
        && catalog(*p).selectedId.isEmpty(), "explicit CUSTOM file import clears prior origin");
    restore(*p, project);
    const auto message = VDX7Sysex::encode(selected.packed);
    midi.addEvent(juce::MidiMessage(message.data(), int(message.size())), 0);
    p->processBlock(audio, midi);
    require(catalog(*p).selectedId.isEmpty() && oldHandle->banks.size() == 2, "live MIDI bank replacement clears origin and keeps immutable library");
}

int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    try
    {
        require(argc == 1 || argc == 2, "optional local verified ROM path only");
        if (argc == 2) testWithRom(juce::File(argv[1]));
        else { testNoRom(); testBoundaries(); }
        std::cout << "PASS: imported catalog processor ownership, binary state and bounded scheduling\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        scanBoundary = {}; saveBoundary = {};
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
