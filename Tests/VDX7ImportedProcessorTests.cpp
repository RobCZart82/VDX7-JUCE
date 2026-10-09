#include "PluginProcessor.h"
#include "VDX7InitVoice.h"
#include "VDX7Sysex.h"
#include <juce_cryptography/juce_cryptography.h>
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>

static std::function<void()> scanBoundary, saveBoundary, selectionBoundary;
void vdx7TestImportedBankScanBoundary()
{ auto action = std::move(scanBoundary); scanBoundary = {}; if (action) action(); }
void vdx7TestRomStateBoundary()
{ auto action = std::move(saveBoundary); saveBoundary = {}; if (action) action(); }
void vdx7TestStateBoundary(std::size_t) {}
void vdx7TestImportedBankSelectionBoundary()
{ auto action = std::move(selectionBoundary); selectionBoundary = {}; if (action) action(); }
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

static void testParameterSnapshot(const juce::File& rom = {})
{
    auto p = std::make_unique<VDX7AudioProcessor>(false);
    if (rom.existsAsFile()) require(p->loadRomFromFile(rom), "snapshot private ROM control");
    const auto set = [&](const char* id, float value) {
        auto* parameter = p->parameters().getParameter(id);
        require(parameter != nullptr, "snapshot parameter exists");
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
    };
    const auto seed = [&] {
        set(VDX7ParameterIDs::masterVolume, -12.0f);
        set(VDX7ParameterIDs::pitchWheel, -0.5f);
        set(VDX7ParameterIDs::modWheel, 0.25f);
        setFeedback(*p, 2);
        require(p->setMidiInputChannelFromUi(3), "seed snapshot routing");
    };
    seed();
    const auto before = save(*p);
    auto incoming = tree(before);
    incoming.setProperty("midiInputChannel", 9, nullptr);
    auto incomingParameters = incoming.getChildWithName("PARAMETERS");
    require(incomingParameters.getNumChildren() == 148, "snapshot retains all host parameters");
    for (const auto& id : {VDX7ParameterIDs::masterVolume, VDX7ParameterIDs::pitchWheel,
                          VDX7ParameterIDs::modWheel})
        incomingParameters.getChildWithProperty("id", id).setProperty("value", 0.0f, nullptr);
    const auto nextProject = binary(incoming);

    // The final edit also covers the preserved-pending path without a ROM,
    // after the preceding iteration installed a missing-firmware project.
    for (const bool replaceProject : {false, true, false})
    {
        seed();
        const auto expected = tree(save(*p)).getChildWithName("PARAMETERS");
        saveBoundary = [&] {
            if (replaceProject) restore(*p, nextProject);
            else {
                set(VDX7ParameterIDs::masterVolume, -3.0f);
                set(VDX7ParameterIDs::pitchWheel, 0.5f);
                set(VDX7ParameterIDs::modWheel, 0.75f);
                setFeedback(*p, 6);
                require(p->setMidiInputChannelFromUi(9), "later snapshot routing edit");
            }
        };
        const auto captured = tree(save(*p));
        const auto actual = captured.getChildWithName("PARAMETERS");
        require(int(captured["midiInputChannel"]) == 3, "snapshot retains captured routing");
        require(actual.getNumChildren() == expected.getNumChildren(), "snapshot parameter count unchanged");
        for (const auto child : expected)
            require(actual.getChildWithProperty("id", child["id"])["value"] == child["value"],
                "save must not read parameter values from a later edit or project restore");
        require(p->getMidiInputChannel() == 9, "detached serialization does not undo later project edits");
        require(std::abs(p->parameters().getRawParameterValue(VDX7ParameterIDs::masterVolume)->load()
                         - (replaceProject ? 0.0f : -3.0f)) < 0.001f,
            "detached serialization does not undo later live volume");
        auto reopened = std::make_unique<VDX7AudioProcessor>(false);
        restore(*reopened, binary(captured));
        const auto recalled = tree(save(*reopened)).getChildWithName("PARAMETERS");
        for (const auto child : expected)
            require(recalled.getChildWithProperty("id", child["id"])["value"] == child["value"],
                "captured parameters survive actual processor binary recall");
        require(reopened->getMidiInputChannel() == 3, "captured routing survives binary recall");
    }
    std::cout << "PASS: detached processor save retains captured host values across edits and project restore\n";
}

struct VDX7RegressionAccess
{
    static auto restoreGeneration(VDX7AudioProcessor& p)
    {
        std::scoped_lock lock(p.engineMutex_);
        return std::make_pair(p.midiTimelineEpoch_.load(), p.importedBankRevision_);
    }
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

static void testParameterAdmission(const juce::File& rom = {})
{
    Fixtures fixtures;
    const auto folder = fixtures.folder("parameter-admission");
    put(folder, syntheticBank(480));
    auto p = std::make_unique<VDX7AudioProcessor>(false, juce::File(), folder);
    if (rom.existsAsFile()) require(p->loadRomFromFile(rom), "admission private ROM control");
    p->prepareToPlay(48000, 64);
    auto* volume = p->parameters().getParameter(VDX7ParameterIDs::masterVolume);
    volume->setValueNotifyingHost(volume->convertTo0to1(-12.0f));
    require(p->setMidiInputChannelFromUi(3), "seed admission routing");
    const auto valid = save(*p);

    // Repeat after a genuine restore to cover both live and pending payloads.
    for (const bool pending : {false, true})
    {
        if (pending) restore(*p, valid);
        const auto before = save(*p);
        const auto snapshot = p->getImportedBankSnapshot();
        const auto generation = VDX7RegressionAccess::restoreGeneration(*p);
        const auto checkRejected = [&](juce::ValueTree incoming) {
            incoming.setProperty("midiInputChannel", 9, nullptr);
            incoming.setProperty("monoNoteZeroCorrection", true, nullptr);
            incoming.removeChild(incoming.getChildWithName("ImportedBanks"), nullptr);
            restore(*p, binary(incoming));
            require(p->getMidiInputChannel() == 3 && p->getImportedBankSnapshot() == snapshot,
                "invalid parameters preserve routing and exact imported catalog ownership");
            require(VDX7RegressionAccess::restoreGeneration(*p) == generation,
                "invalid parameters do not advance MIDI epoch or catalog revision");
            require(save(*p) == before, "invalid parameters preserve entire saved project and pending payload");
            for (const auto child : tree(before).getChildWithName("PARAMETERS"))
            {
                const auto actual = p->parameters().getRawParameterValue(child["id"].toString())->load();
                require(std::isfinite(actual), "rejected restore leaves every live host value finite");
            }
        };
        // NaN must be rejected for every host ID, not just the three global controls.
        for (const auto child : tree(before).getChildWithName("PARAMETERS"))
        {
            auto incoming = tree(before);
            incoming.getChildWithName("PARAMETERS").getChildWithProperty("id", child["id"])
                .setProperty("value", "nan", nullptr);
            checkRejected(incoming);
        }
        for (const auto* bad : {"NaN", "inf", "-inf", "1e999", "1e39", "", "garbage", "0.5junk",
                               "1e", "+", ".", "0x1", " 0.5", "0.5 "})
            for (const auto* id : {VDX7ParameterIDs::masterVolume, VDX7ParameterIDs::pitchWheel,
                                  VDX7ParameterIDs::modWheel})
            {
                auto incoming = tree(before);
                incoming.getChildWithName("PARAMETERS").getChildWithProperty("id", id)
                    .setProperty("value", bad, nullptr);
                checkRejected(incoming);
            }
        auto duplicate = tree(before);
        auto parameters = duplicate.getChildWithName("PARAMETERS");
        parameters.addChild(parameters.getChild(0).createCopy(), -1, nullptr);
        checkRejected(duplicate);
        auto competing = tree(before);
        competing.addChild(competing.getChildWithName("PARAMETERS").createCopy(), -1, nullptr);
        checkRejected(competing);
        auto wrongType = tree(before);
        auto known = wrongType.getChildWithName("PARAMETERS").getChild(0);
        juce::ValueTree badChild("OTHER");
        badChild.setProperty("id", known["id"], nullptr);
        badChild.setProperty("value", 0, nullptr);
        wrongType.getChildWithName("PARAMETERS").addChild(badChild, -1, nullptr);
        checkRejected(wrongType);
    }

    auto positive = tree(valid);
    auto parameters = positive.getChildWithName("PARAMETERS");
    parameters.getChildWithProperty("id", VDX7ParameterIDs::masterVolume).setProperty("value", "-1.25e1", nullptr);
    parameters.getChildWithProperty("id", VDX7ParameterIDs::pitchWheel).setProperty("value", "-.5", nullptr);
    parameters.getChildWithProperty("id", VDX7ParameterIDs::modWheel).setProperty("value", "+.25", nullptr);
    positive.setProperty("midiInputChannel", 9, nullptr);
    restore(*p, binary(positive));
    require(p->getMidiInputChannel() == 9
        && std::abs(p->parameters().getRawParameterValue(VDX7ParameterIDs::masterVolume)->load() + 12.5f) < 0.01f
        && std::abs(p->parameters().getRawParameterValue(VDX7ParameterIDs::pitchWheel)->load() + 0.5f) < 0.001f
        && std::abs(p->parameters().getRawParameterValue(VDX7ParameterIDs::modWheel)->load() - 0.25f) < 0.001f,
        "complete decimal/scientific values reach actual processor restore");
    // Backward compatibility: finite values still use the existing range clamp.
    auto clamped = tree(valid);
    clamped.getChildWithName("PARAMETERS").getChildWithProperty("id", VDX7ParameterIDs::modWheel)
        .setProperty("value", 2.0f, nullptr);
    restore(*p, binary(clamped));
    require(p->parameters().getRawParameterValue(VDX7ParameterIDs::modWheel)->load() == 1.0f,
        "finite out-of-range values retain legacy clamping");
    auto defaulted = tree(valid);
    auto defaultParameters = defaulted.getChildWithName("PARAMETERS");
    defaultParameters.getChildWithProperty("id", VDX7ParameterIDs::modWheel).removeProperty("value", nullptr);
    juce::ValueTree unknown("PARAM");
    unknown.setProperty("id", "futureParameter", nullptr);
    unknown.setProperty("value", "nan", nullptr); // APVTS never consumes unknown IDs.
    defaultParameters.addChild(unknown, -1, nullptr);
    defaulted.setProperty("midiInputChannel", 8, nullptr);
    restore(*p, binary(defaulted));
    require(p->getMidiInputChannel() == 8
        && p->parameters().getRawParameterValue(VDX7ParameterIDs::modWheel)->load() == 0.0f,
        "missing values use defaults and unknown IDs remain ignored");
    auto partial = tree(valid);
    partial.getChildWithName("PARAMETERS").removeAllChildren(nullptr);
    partial.setProperty("midiInputChannel", 7, nullptr);
    restore(*p, binary(partial));
    require(p->getMidiInputChannel() == 7, "empty legacy parameter tree reaches restore");
    auto legacy = tree(valid);
    legacy.removeChild(legacy.getChildWithName("PARAMETERS"), nullptr);
    restore(*p, binary(legacy));
    require(p->getMidiInputChannel() == 3, "partial and RAM-only legacy snapshots remain admissible");
    juce::AudioBuffer<float> audio(2, 64);
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(3, 60, juce::uint8(100)), 0);
    p->processBlock(audio, midi);
    for (int channel = 0; channel < audio.getNumChannels(); ++channel)
        for (int sample = 0; sample < audio.getNumSamples(); ++sample)
            require(std::isfinite(audio.getSample(channel, sample)), "post-admission render stays finite");
    p->releaseResources();
    std::cout << "PASS: malformed host parameter snapshots rejected before all project mutation\n";
}

static void testStartup()
{
    Fixtures fixtures;
    const auto folder = fixtures.folder("startup");
    const auto first = syntheticBank(400), second = syntheticBank(401);
    put(folder, first); put(folder, second);
    auto p = std::make_unique<VDX7AudioProcessor>(false, juce::File(), folder);
    const auto initial = p->getImportedBankSnapshot();
    require(initial && initial->banks.size() == 2,
        "new instance scans injected imported folder before any explicit refresh");
    require(initial->banks[0].packed == first.packed && initial->selectedId.isEmpty()
        && p->getImportedBankSelection().index == -1 && !p->isRomLoaded(),
        "startup builds an exact unselected catalog without firmware or working-bank changes");
    const auto before = save(*p);
    const auto third = syntheticBank(402);
    put(folder, third);
    p->prepareToPlay(48000, 64);
    juce::AudioBuffer<float> audio(2, 64);
    juce::MidiBuffer midi;
    p->processBlock(audio, midi);
    p->releaseResources();
    require(p->getImportedBankSnapshot() == initial && save(*p) == before,
        "prepare/process/release never rescan or modify startup library");
    auto q = std::make_unique<VDX7AudioProcessor>(false, juce::File(), folder);
    require(q->getImportedBankSnapshot()->banks.size() == 3 && initial->banks.size() == 2,
        "new instances see new files while existing instances retain isolated catalogs");
    restore(*q, before);
    require(q->getImportedBankSnapshot()->banks.size() == 2
        && catalog(*q).banks[0].packed == first.packed,
        "restored project replaces startup catalog without reading newer disk banks");
    auto legacy = tree(before);
    legacy.removeChild(legacy.getChildWithName("ImportedBanks"), nullptr);
    restore(*q, binary(legacy));
    require(!q->getImportedBankSnapshot(), "legacy project absence overrides startup catalog too");
    juce::String report;
    require(p->refreshImportedBanks(folder, report) && p->getImportedBankSnapshot()->banks.size() == 3,
        "existing instance sees added files only on explicit Refresh");
    require(p->getImportedBankStartupReport().contains("2 bank(s)")
        && !p->getStatusText().contains("startup warning"), "startup report remains historical after Refresh");

    const auto missing = fixtures.root.getChildFile("missing");
    auto empty = std::make_unique<VDX7AudioProcessor>(false, juce::File(), missing);
    require(empty->getImportedBankSnapshot() && empty->getImportedBankSnapshot()->banks.empty()
        && !missing.exists() && !empty->getStatusText().contains("startup warning"),
        "missing folder is a complete empty scan and is never created at startup");
    auto pathError = std::make_unique<VDX7AudioProcessor>(false, juce::File(), folder.getChildFile(first.fileName));
    require(!pathError->getImportedBankSnapshot()
        && pathError->getImportedBankStartupReport().contains("Scan incomplete")
        && pathError->getStatusText().contains("startup warning"), "bad startup path preserves unscanned state with diagnostics");
    require(!pathError->refreshImportedBanks(folder.getChildFile(first.fileName), report)
        && pathError->getStatusText().contains("startup warning"), "failed Refresh cannot dismiss startup warning");
    const auto excessive = fixtures.folder("startup-limit");
    for (int n = 0; n < 129; ++n)
    {
        auto alias = first; alias.fileName = juce::String(n) + ".syx"; put(excessive, alias);
    }
    auto limited = std::make_unique<VDX7AudioProcessor>(false, juce::File(), excessive);
    require(!limited->getImportedBankSnapshot()
        && limited->getImportedBankStartupReport().contains("Scan incomplete"),
        "startup file limit never publishes a partial catalog");
    const auto noisy = fixtures.folder("startup-warnings");
    put(noisy, first);
    auto alias = first; alias.fileName = "alias.syx"; put(noisy, alias);
    for (int n = 0; n < 12; ++n)
        require(noisy.getChildFile("broken-" + juce::String(n) + ".syx").replaceWithData("invalid", 7), "write rejected synthetic fixture");
    auto warnings = std::make_unique<VDX7AudioProcessor>(false, juce::File(), noisy);
    require(warnings->getImportedBankSnapshot() && warnings->getImportedBankSnapshot()->banks.size() == 1
        && warnings->getImportedBankStartupReport().contains("5 additional warnings")
        && warnings->getStatusText().contains("startup warning"),
        "valid startup subset publishes with bounded duplicate and invalid-file warnings");
    require(warnings->getImportedBankStartupReport().getNumBytesAsUTF8() < 2048,
        "startup report stays bounded for diagnostic display");
    const auto historical = warnings->getImportedBankStartupReport();
    require(warnings->refreshImportedBanks(folder, report)
        && !warnings->getStatusText().contains("startup warning")
        && warnings->getImportedBankStartupReport() == historical, "successful Refresh clears marker but preserves historical report");
    require(!tree(save(*warnings)).hasProperty("ImportedBankStartupReport"), "startup diagnostics are not project state");
}

static void testNoRom()
{
    Fixtures fixtures;
    const auto folder = fixtures.folder("library");
    const auto first = syntheticBank(200), second = syntheticBank(201);
    put(folder, first); put(folder, second);
    auto p = std::make_unique<VDX7AudioProcessor>(false);
    require(!p->getImportedBankSnapshot() && !tree(save(*p)).getChildWithName("ImportedBanks").isValid(),
        "isolated processor without injected folder does not scan/create an owner library");
    require(p->getImportedBankStartupReport().isEmpty(), "isolated processor has no synthetic startup report");
    juce::String report;
    require(p->refreshImportedBanks(folder, report), "explicit pre-ROM catalog refresh");
    const auto before = p->getImportedBankSnapshot();
    require(before && before->banks.size() == 2 && before->selectedId.isEmpty(), "immutable catalog has no fabricated live origin");
    const auto unloadedState = save(*p);
    require(!p->selectImportedBank(before, first.contentId, 0, report)
        && report.contains("firmware first") && save(*p) == unloadedState,
        "selection without firmware leaves state and catalog unchanged");
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
    const auto pendingState = save(*q);
    require(!q->selectImportedBank(kept, first.contentId, 0, report)
        && report.contains("Project preserved") && save(*q) == pendingState,
        "pending project blocks imported selection without consuming edits");
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
    const auto beforeSelection = p->getImportedBankSnapshot();
    selectionBoundary = [&] { require(p->refreshImportedBanks(b, report), "new catalog during selection lookup"); };
    require(!p->selectImportedBank(beforeSelection, syntheticBank(200).contentId, 0, report)
        && report.contains("library changed") && catalog(*p).banks[0].contentId == syntheticBank(201).contentId,
        "ROM-free second token check detects lookup-to-transaction replacement before firmware admission");
    require(p->refreshImportedBanks(a, report), "reset isolated boundary catalog");
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
    const auto pendingSelectionToken = p->getImportedBankSnapshot();
    selectionBoundary = [&] { restore(*p, stateA); };
    require(!p->selectImportedBank(pendingSelectionToken, syntheticBank(201).contentId, 0, report)
        && report.contains("library changed") && catalog(*p).banks[0].contentId == syntheticBank(200).contentId,
        "project restored after lookup cannot be overwritten by old imported choice");
    restore(*p, otherState);
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
    auto p = std::make_unique<VDX7AudioProcessor>(false, juce::File(), folder);
    const auto startup = p->getImportedBankSnapshot();
    require(startup && startup->banks.size() == 2, "private-ROM path starts with unselected startup catalog");
    p->prepareToPlay(48000, 128);
    require(p->loadRomFromFile(rom), "private verified firmware");
    require(p->getImportedBankSnapshot() == startup && p->getImportedBankSelection().index == -1,
        "ROM load preserves startup library and never auto-selects its working bank");
    require(p->setControllerSettingFromUi(0, 0, 42) && p->setPlaySettingFromUi(3, 63)
        && p->setPitchBendSettingFromUi(0, 7) && p->setMasterTuneFromUi(7)
        && p->setMidiInputChannelFromUi(3), "non-default settings before imported selection");
    juce::String report;
    require(p->refreshImportedBanks(folder, report), "loaded explicit catalog refresh");
    setFeedback(*p, 6);
    save(*p); // Apply the queued edit before the preservation oracle.
    const auto oldRam = VDX7RegressionAccess::ram(*p);
    require(p->refreshImportedBanks(folder, report) && VDX7RegressionAccess::ram(*p) == oldRam
        && p->isCurrentVoiceModified(), "loaded refresh preserves all engine RAM and dirty working voice");
    const auto selectionToken = p->getImportedBankSnapshot();
    const auto unselectedState = save(*p);
    const auto controllers = p->getControllerSettings();
    const auto play = p->getPlaySettings();
    const auto bend = p->getPitchBendSettings();
    VDX7FactoryBanks::Snapshot factoryBefore;
    bool factoryPresent = false;
    require(VDX7FactoryBanks::readState(tree(unselectedState), factoryBefore, factoryPresent) && factoryPresent,
        "capture independent factory catalog before imported selection");
    const auto impostor = std::make_shared<const VDX7ImportedBanks::Snapshot>(*selectionToken);
    for (int program : {-1, 32})
        require(!p->selectImportedBank(selectionToken, selected.contentId, program, report), "out-of-range program rejected");
    require(!p->selectImportedBank({}, selected.contentId, 0, report)
        && !p->selectImportedBank(impostor, selected.contentId, 0, report)
        && !p->selectImportedBank(selectionToken, "not-a-bank", 0, report)
        && save(*p) == unselectedState, "invalid token/id/program preserves complete prior state");
    selectionBoundary = [&] { require(p->refreshImportedBanks(folder, report), "refresh during selection lookup"); };
    require(!p->selectImportedBank(selectionToken, selected.contentId, 7, report)
        && report.contains("library changed") && save(*p) == unselectedState,
        "stale token checked again inside RAM transaction, before flushing or importing");
    const auto currentToken = p->getImportedBankSnapshot();
    require(!p->selectImportedBank(selectionToken, selected.contentId, 7, report), "stale displayed catalog rejected");
    require(p->selectFactoryBank(0), "older queued factory switch before imported transaction");
    p->selectProgramFromUi(2);
    setFeedback(*p, 5);
    require(p->selectImportedBank(currentToken, selected.contentId, 7, report)
        && p->getCurrentBank() == -1 && p->getCurrentProgram() == 7 && !p->isCurrentVoiceModified()
        && catalog(*p).selectedId == selected.contentId
        && std::equal(selected.packed.begin(), selected.packed.end(), VDX7RegressionAccess::ram(*p).begin()),
        "actual selection installs exact bank bytes and requested program, without migrating older queued edits");
    VDX7FactoryBanks::Snapshot factoryAfter;
    require(p->getControllerSettings() == controllers && p->getPlaySettings() == play
        && p->getPitchBendSettings() == bend && p->getMasterTune() == 7 && p->getMidiInputChannel() == 3
        && VDX7FactoryBanks::readState(tree(save(*p)), factoryAfter, factoryPresent)
        && factoryAfter.image == factoryBefore.image && factoryAfter.mask == factoryBefore.mask,
        "imported selection preserves performance/settings and original factory catalog");
    setFeedback(*p, 6);
    auto edited = tree(save(*p));
    const auto before = VDX7RegressionAccess::ram(*p);
    require(p->isCurrentVoiceModified() && catalog(*p).selectedId == selected.contentId
        && currentToken->banks[0].packed == selected.packed,
        "working edit retains imported origin without modifying immutable source bank");
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
        && reopened->getCurrentProgram() == 7 && reopened->getMasterTune() == 7
        && reopened->getMidiInputChannel() == 3
        && std::equal(before.begin(), before.begin() + 4096, VDX7RegressionAccess::ram(*reopened).begin()),
        "actual processor binary recall is independent of deleted source bank folder");
    juce::AudioBuffer<float> audio(2, 128); juce::MidiBuffer midi;
    auto unavailable = edited.createCopy();
    unavailable.setProperty("romPath", fixtures.root.getChildFile("missing-firmware.bin").getFullPathName(), nullptr);
    auto waiting = std::make_unique<VDX7AudioProcessor>(false);
    restore(*waiting, binary(unavailable));
    require(!waiting->isProjectReady() && catalog(*waiting).selectedId == selected.contentId,
        "missing firmware preserves imported origin and working RAM as pending project");
    const auto waitingState = save(*waiting);
    require(!waiting->selectImportedBank(waiting->getImportedBankSnapshot(), selected.contentId, 0, report)
        && save(*waiting) == waitingState, "imported selection cannot discard a pending edited project");
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
    restore(*p, project);
    scanBoundary = [&] {
        require(p->selectImportedBank(p->getImportedBankSnapshot(), syntheticBank(201).contentId, 3, report),
            "new imported selection during detached scan");
    };
    require(!p->refreshImportedBanks(folder, report) && report.contains("changed during scan")
        && catalog(*p).selectedId == syntheticBank(201).contentId && p->getCurrentProgram() == 3,
        "imported selection invalidates prior-origin scan and does not reopen missing source file");
    p->selectProgramFromUi(17);
    midi.clear();
    p->processBlock(audio, midi);
    require(p->getCurrentProgram() == 17 && catalog(*p).selectedId == syntheticBank(201).contentId,
        "ordinary program selection retains imported bank origin");
    VDX7AudioProcessor::WorkingVoiceSnapshot initExpected;
    require(p->captureWorkingVoiceSnapshot(initExpected, report)
        && p->initialiseVoiceFromUi(initExpected.voice, initExpected.program, initExpected.revision, report)
        && catalog(*p).selectedId.isEmpty() && p->getImportedBankSnapshot()->banks.size() == 2,
        "confirmed Init creates new CUSTOM working patch without deleting imported bank library");
}

int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    try
    {
        if (argc >= 2 && juce::String(argv[1]) == "--snapshot-only")
        {
            require(argc == 2 || argc == 3, "snapshot-only accepts an optional private ROM");
            if (argc == 3) require(juce::File(argv[2]).existsAsFile(), "explicit snapshot ROM exists");
            testParameterSnapshot(argc == 3 ? juce::File(argv[2]) : juce::File());
            testParameterAdmission(argc == 3 ? juce::File(argv[2]) : juce::File());
            return 0;
        }
        require(argc == 1 || argc == 2, "optional local verified ROM path only");
        if (argc == 2) { testParameterSnapshot(juce::File(argv[1])); testParameterAdmission(juce::File(argv[1])); testWithRom(juce::File(argv[1])); }
        else { testParameterSnapshot(); testParameterAdmission(); testStartup(); testNoRom(); testBoundaries(); }
        std::cout << "PASS: imported catalog processor ownership, binary state and bounded scheduling\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        scanBoundary = {}; saveBoundary = {}; selectionBoundary = {};
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
