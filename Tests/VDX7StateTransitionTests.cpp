// Deterministic state-save/ROM-install regressions. Private compatible ROM only;
// the public CI target compiles this test but never supplies firmware.
#include "PluginProcessor.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <future>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>

struct VDX7RegressionAccess
{
    static void listenerCallback(VDX7AudioProcessor& processor, const juce::String& id, int value)
    { processor.parameterChanged(id, float(value)); }

    static void pollPublication(VDX7AudioProcessor& processor)
    {
        // Same admission as the processor timer, without requiring a running
        // native message loop or a state save that would force synchronization.
        if (processor.voicePublicationNeeded_.load(std::memory_order_acquire)
            || processor.editQueue_.pending())
            processor.synchroniseOperatorParametersFromEngine();
    }
};

namespace
{
void require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

struct Gate
{
    std::atomic<bool> entered {false}, resume {false}, timedOut {false};
    int stage = -1;

    void pause()
    {
        entered.store(true, std::memory_order_release);
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
        while (!resume.load(std::memory_order_acquire))
        {
            if (std::chrono::steady_clock::now() >= deadline)
            {
                timedOut.store(true, std::memory_order_release);
                return;
            }
            std::this_thread::yield();
        }
    }

    void awaitEntry()
    {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (!entered.load(std::memory_order_acquire)
               && std::chrono::steady_clock::now() < deadline)
            std::this_thread::yield();
        if (!entered.load(std::memory_order_acquire))
        {
            resume.store(true, std::memory_order_release);
            require(false, "deterministic state transition hook was not reached");
        }
    }
};

thread_local Gate* saveGate = nullptr;
thread_local Gate* transitionGate = nullptr;

// Destruction resumes the worker before std::future destruction waits, so a
// failed main-thread assertion cannot strand a worker inside a scheduling hook.
struct ResumeOnExit
{
    Gate& gate;
    ~ResumeOnExit() { gate.resume.store(true, std::memory_order_release); }
};

juce::ValueTree decode(const juce::MemoryBlock& bytes)
{
    const auto xml = juce::AudioProcessor::getXmlFromBinary(bytes.getData(), int(bytes.getSize()));
    require(xml != nullptr, "decode state XML");
    auto tree = juce::ValueTree::fromXml(*xml);
    require(tree.isValid(), "decode state tree");
    return tree;
}

juce::MemoryBlock save(VDX7AudioProcessor& processor)
{
    juce::MemoryBlock state;
    processor.getStateInformation(state);
    return state;
}

void set(VDX7AudioProcessor& processor, const juce::String& id, int value)
{
    auto* parameter = processor.parameters().getParameter(id);
    require(parameter != nullptr, "host parameter exists");
    parameter->setValueNotifyingHost(parameter->convertTo0to1(float(value)));
}

struct Values { int feedback, op1, op6; };
constexpr Values initial {2, 13, 24};
constexpr Values edited {6, 42, 47};
constexpr Values latest {5, 43, 48};

void setValues(VDX7AudioProcessor& processor, Values values)
{
    set(processor, VDX7ParameterIDs::voiceParameter(VDX7VoiceData::VoiceParameter::feedback), values.feedback);
    set(processor, VDX7ParameterIDs::operatorParameter(0, VDX7VoiceData::Parameter::outputLevel), values.op1);
    set(processor, VDX7ParameterIDs::operatorParameter(5, VDX7VoiceData::Parameter::outputLevel), values.op6);
}

void verifyValues(VDX7AudioProcessor& processor, Values expected)
{
    const auto tree = decode(save(processor));
    juce::MemoryBlock ram;
    require(ram.fromBase64Encoding(tree.getProperty("ram").toString())
            && ram.getSize() == VDX7Engine::kRamStateSize, "decode installed project RAM");
    const auto* voice = static_cast<const uint8_t*>(ram.getData())
                      + int(tree.getProperty("program", 0)) * VDX7VoiceData::kPackedVoiceSize;
    require(VDX7VoiceData::getVoiceParameter(voice, 128, VDX7VoiceData::VoiceParameter::feedback)
                == expected.feedback, "state transition preserves latest voice edit");
    require(VDX7VoiceData::getOperatorParameter(voice, 128, 0, VDX7VoiceData::Parameter::outputLevel)
                == expected.op1, "state transition preserves low-mask operator edit");
    require(VDX7VoiceData::getOperatorParameter(voice, 128, 5, VDX7VoiceData::Parameter::outputLevel)
                == expected.op6, "state transition preserves high-mask operator edit");
    const auto hostValue = [&](const juce::String& id)
    { return juce::roundToInt(processor.parameters().getRawParameterValue(id)->load()); };
    require(hostValue(VDX7ParameterIDs::voiceParameter(VDX7VoiceData::VoiceParameter::feedback)) == expected.feedback
            && hostValue(VDX7ParameterIDs::operatorParameter(0, VDX7VoiceData::Parameter::outputLevel)) == expected.op1
            && hostValue(VDX7ParameterIDs::operatorParameter(5, VDX7VoiceData::Parameter::outputLevel)) == expected.op6,
            "host publication preserves installed transition edits");
}

void verifyDeferred(const juce::MemoryBlock& state, Values expected)
{
    const auto tree = decode(state);
    const auto edits = tree.getChildWithName("DeferredVoiceEdits");
    require(edits.isValid(), "no-ROM save retains detached deferred edits");
    require(int(edits.getProperty("voice9", -1)) == expected.feedback
            && int(edits.getProperty("op8", -1)) == expected.op1
            && int(edits.getProperty("op113", -1)) == expected.op6,
            "no-ROM save retains both operator masks and voice edit");
}

void reopen(const juce::MemoryBlock& state, const juce::File& rom, Values expected)
{
    auto processor = std::make_unique<VDX7AudioProcessor>(false);
    processor->setStateInformation(state.getData(), int(state.getSize()));
    if (!processor->isProjectReady())
        require(processor->loadRomFromFile(rom), "install reopened project's matching ROM");
    verifyValues(*processor, expected);
}

void testNoRomSave(const juce::File& rom)
{
    for (const bool loadBeforeSave : {false, true})
    {
        auto processor = std::make_unique<VDX7AudioProcessor>(false);
        setValues(*processor, edited);
        if (loadBeforeSave) require(processor->loadRomFromFile(rom), "control first ROM load");
        const auto state = save(*processor);
        if (!loadBeforeSave)
        {
            verifyDeferred(state, edited);
            require(processor->loadRomFromFile(rom), "control first ROM after save");
        }
        verifyValues(*processor, edited);
        reopen(state, rom, edited);
    }

    auto processor = std::make_unique<VDX7AudioProcessor>(false);
    setValues(*processor, edited);
    Gate gate;
    auto saving = std::async(std::launch::async, [&] {
        saveGate = &gate;
        const auto state = save(*processor);
        saveGate = nullptr;
        return state;
    });
    ResumeOnExit release {gate};
    gate.awaitEntry();
    // The detached save belongs to the earlier generation. A newer edit must
    // reach the first installed voice without relabelling the saved values.
    setValues(*processor, latest);
    require(processor->loadRomFromFile(rom), "overlapping first ROM load");
    gate.resume.store(true, std::memory_order_release);
    const auto state = saving.get();
    require(!gate.timedOut.load(std::memory_order_acquire), "save scheduling hook did not time out");
    require(decode(state).getProperty("ram").toString().isEmpty(), "save captured original no-ROM generation");
    verifyDeferred(state, edited);
    verifyValues(*processor, latest);
    reopen(state, rom, edited);
    std::cout << "PASS: first ROM load overlapping state save preserves voice and both operator masks\n";
}

void testFreshFirstRom(const juce::File& rom)
{
    for (const int stage : {0, 1})
    {
        std::cout << "CASE: fresh first ROM, edit at transition stage " << stage << std::endl;
        auto processor = std::make_unique<VDX7AudioProcessor>(false);
        setValues(*processor, edited);
        Gate gate;
        gate.stage = stage;
        auto loading = std::async(std::launch::async, [&] {
            transitionGate = &gate;
            const bool result = processor->loadRomFromFile(rom);
            transitionGate = nullptr;
            return result;
        });
        ResumeOnExit release {gate};
        gate.awaitEntry();
        setValues(*processor, latest);
        gate.resume.store(true, std::memory_order_release);
        require(loading.get(), "fresh first ROM load completed");
        require(!gate.timedOut.load(std::memory_order_acquire), "first ROM scheduling hook did not time out");
        verifyValues(*processor, latest);
        reopen(save(*processor), rom, latest);
    }
    std::cout << "PASS: first ROM load without a pending project keeps latest host edits\n";
}

juce::MemoryBlock pendingState(const juce::File& rom, bool withRam)
{
    auto source = std::make_unique<VDX7AudioProcessor>(false);
    if (withRam) require(source->loadRomFromFile(rom), "create loaded project fixture");
    setValues(*source, initial);
    auto state = save(*source);
    if (withRam)
    {
        auto tree = decode(state);
        tree.setProperty("romPath", juce::File::getSpecialLocation(juce::File::tempDirectory)
            .getNonexistentChildFile("vdx7-missing-transition-rom-", ".bin", false).getFullPathName(), nullptr);
        juce::AudioProcessor::copyXmlToBinary(*tree.createXml(), state);
    }
    return state;
}

void testPendingRestore(const juce::File& rom)
{
    for (const bool withRam : {false, true})
    {
        const auto state = pendingState(rom, withRam);
        for (const bool editBeforeLoad : {true, false})
        {
            auto processor = std::make_unique<VDX7AudioProcessor>(false);
            processor->setStateInformation(state.getData(), int(state.getSize()));
            require(!processor->isProjectReady(), "matching ROM is pending in control fixture");
            if (editBeforeLoad) setValues(*processor, edited);
            require(processor->loadRomFromFile(rom), "control matching ROM load");
            if (!editBeforeLoad) setValues(*processor, edited);
            verifyValues(*processor, edited);
        }

        for (const int stage : {0, 1, 2, 3})
        {
            std::cout << "CASE: pending restore " << (withRam ? "with RAM" : "without RAM")
                      << ", edit at transition stage " << stage << std::endl;
            auto processor = std::make_unique<VDX7AudioProcessor>(false);
            processor->setStateInformation(state.getData(), int(state.getSize()));
            setValues(*processor, edited);
            Gate gate;
            gate.stage = stage;
            auto loading = std::async(std::launch::async, [&] {
                transitionGate = &gate;
                const bool result = processor->loadRomFromFile(rom);
                transitionGate = nullptr;
                return result;
            });
            ResumeOnExit release {gate};
            gate.awaitEntry();
            require(!processor->isProjectReady(), "project stays pending until installation completes");
            setValues(*processor, latest);
            gate.resume.store(true, std::memory_order_release);
            require(loading.get(), "matching ROM load completed");
            require(!gate.timedOut.load(std::memory_order_acquire), "restore scheduling hook did not time out");
            require(processor->isProjectReady(), "project becomes ready after matching load");
            verifyValues(*processor, latest);
            reopen(save(*processor), rom, latest);
        }
    }
    std::cout << "PASS: pre/during/post pending ROM installation retains latest voice/operator edits with and without RAM\n";
}

void testLateMailboxPublication(const juce::File& rom)
{
    const auto state = pendingState(rom, true);
    for (const int field : {0, 1, 2})
    {
        std::cout << "CASE: pending-route listener resumes after publication, field " << field << std::endl;
        auto processor = std::make_unique<VDX7AudioProcessor>(false);
        processor->setStateInformation(state.getData(), int(state.getSize()));
        processor->prepareToPlay(48000.0, 64);
        const auto id = field == 0
            ? VDX7ParameterIDs::voiceParameter(VDX7VoiceData::VoiceParameter::feedback)
            : VDX7ParameterIDs::operatorParameter(field == 1 ? 0 : 5, VDX7VoiceData::Parameter::outputLevel);
        const int desired = field == 0 ? latest.feedback : (field == 1 ? latest.op1 : latest.op6);
        Gate gate;
        gate.stage = field == 0 ? 5 : 4;
        auto callback = std::async(std::launch::async, [&] {
            transitionGate = &gate;
            // Isolate our listener-routing contract: ordinary JUCE adapter
            // callbacks hold an additional listener mutex. The friend call
            // invokes the same production callback without that unrelated
            // mutex preventing the loader's publication from completing.
            VDX7RegressionAccess::listenerCallback(*processor, id, desired);
            transitionGate = nullptr;
        });
        ResumeOnExit release {gate};
        gate.awaitEntry();
        require(processor->loadRomFromFile(rom), "loader finishes while old routing decision is suspended");
        require(processor->isProjectReady(), "loader publishes ready project before listener resumes");
        gate.resume.store(true, std::memory_order_release);
        callback.get();
        require(!gate.timedOut.load(std::memory_order_acquire), "routing scheduling hook did not time out");

        juce::AudioBuffer<float> audio(2, 64);
        juce::MidiBuffer midi;
        processor->processBlock(audio, midi);
        VDX7RegressionAccess::pollPublication(*processor);
        auto* parameter = processor->parameters().getParameter(id);
        require(juce::roundToInt(processor->parameters().getRawParameterValue(id)->load()) == desired
                && juce::roundToInt(parameter->convertFrom0to1(parameter->getValue())) == desired,
                "late mailbox edit is republished without a state-save-forced sync");
        Values expected = initial;
        if (field == 0) expected.feedback = desired;
        else if (field == 1) expected.op1 = desired;
        else expected.op6 = desired;
        verifyValues(*processor, expected);
    }
    std::cout << "PASS: delayed pending-route listeners request publication after ROM installation\n";
}
void testFactoryVoiceValidation(const juce::File& rom)
{
    juce::MemoryBlock image;
    require(rom.loadFileAsData(image) && image.getSize() >= VDX7Engine::kFirmwareSize,
            "read private firmware for bank validation");
    const auto* firmware = static_cast<const uint8_t*>(image.getData());
    std::vector<uint8_t> voices(VDX7Engine::kFactoryVoicesSize, 0);
    for (std::size_t offset = 0; offset < voices.size(); offset += 128)
        std::fill_n(voices.begin() + offset + 118, 10, uint8_t('A'));
    VDX7Engine engine;
    require(engine.loadRomImage(firmware, VDX7Engine::kFirmwareSize, voices.data(), voices.size()),
            "valid synthetic factory bank accepted");
    std::vector<uint8_t> before, after;
    require(engine.saveRam(before), "capture existing engine before invalid bank");
    for (const int slot : {0, 31, 32, 255})
    {
        auto bad = voices;
        bad[slot * 128 + 12] = 15 << 3;
        require(!engine.loadRomImage(firmware, VDX7Engine::kFirmwareSize, bad.data(), bad.size()),
                "invalid optional factory detune rejected before engine mutation");
        require(engine.saveRam(after) && before == after, "failed optional load preserves engine RAM");
        std::vector<uint8_t> combined(firmware, firmware + VDX7Engine::kFirmwareSize);
        combined.insert(combined.end(), bad.begin(), bad.end());
        require(!engine.loadRomImage(combined.data(), combined.size()),
                "invalid combined bank rejected before engine mutation");
        require(engine.saveRam(after) && before == after, "failed combined load preserves engine RAM");
    }
    auto bad = voices;
    bad.back() = 128;
    require(!engine.loadRomImage(firmware, VDX7Engine::kFirmwareSize, bad.data(), bad.size()),
            "non-seven-bit factory name rejected");
    require(engine.saveRam(after) && before == after, "high-bit rejection preserves engine RAM");

    const auto folder = juce::File::getSpecialLocation(juce::File::tempDirectory)
        .getChildFile("vdx7-bank-validation-" + juce::Uuid().toString());
    require(folder.createDirectory().wasOk(), "create private bank-validation fixture directory");
    struct Cleanup { juce::File folder; ~Cleanup() { folder.deleteRecursively(); } } cleanup {folder};
    const auto firmwareFile = folder.getChildFile("firmware.bin");
    const auto companion = folder.getChildFile("dx7_factory_voices_32KB.bin");
    require(firmwareFile.replaceWithData(firmware, VDX7Engine::kFirmwareSize), "write local firmware fixture");
    bad = voices;
    bad[12] = 15 << 3;
    require(companion.replaceWithData(bad.data(), bad.size()), "write invalid synthetic companion");
    VDX7AudioProcessor processor(false);
    require(processor.loadRomFromFile(firmwareFile), "valid firmware survives invalid optional companion");
    require(!processor.hasFactoryVoices() && processor.getStatusText().contains("ignored"),
            "invalid companion is ignored with a visible warning");
    juce::String error;
    require(processor.exportSyx(folder.getChildFile("export.syx"), true, error),
            "remaining valid RAM bank remains exportable");
    const auto state = save(processor);
    VDX7AudioProcessor reopened(false);
    reopened.setStateInformation(state.getData(), int(state.getSize()));
    require(reopened.isProjectReady(), "ignored-companion project state reopens");
    require(companion.replaceWithData(voices.data(), voices.size()), "write valid synthetic companion");
    require(processor.loadRomFromFile(firmwareFile) && processor.hasFactoryVoices(),
            "valid companion accepted as positive control");
    require(processor.exportSyx(folder.getChildFile("valid-export.syx"), true, error),
            "valid companion remains exportable");
    const auto preservedState = save(processor);
    std::vector<uint8_t> invalidCombined(firmware, firmware + VDX7Engine::kFirmwareSize);
    auto invalidVoices = voices;
    invalidVoices[255 * 128 + 12] = 15 << 3;
    invalidCombined.insert(invalidCombined.end(), invalidVoices.begin(), invalidVoices.end());
    const auto combinedFile = folder.getChildFile("invalid-combined.bin");
    require(combinedFile.replaceWithData(invalidCombined.data(), invalidCombined.size()),
            "write invalid synthetic combined image");
    require(!processor.loadRomFromFile(combinedFile, &error)
            && error.startsWith("Invalid combined ROM factory data: Bank 8, voice 32, byte 12")
            && error.contains("detune encoding: value 15; allowed 0..14."),
            "processor rejects entire invalid combined image with visible error");
    require(processor.hasFactoryVoices() && preservedState == save(processor),
            "combined rejection preserves complete saved processor state and factory bank");
    require(processor.exportSyx(folder.getChildFile("preserved-export.syx"), true, error),
            "previous bank remains exportable after combined rejection");
    std::cout << "PASS: factory validation, preservation, warning, export and reopen\n";
}
} // namespace

void vdx7TestStateBoundary(std::size_t) {}
void vdx7TestRomStateBoundary() { if (saveGate != nullptr) saveGate->pause(); }
void vdx7TestRomTransitionBoundary(int stage)
{
    if (transitionGate != nullptr && transitionGate->stage == stage) transitionGate->pause();
}

int main(int argc, char** argv)
{
    try
    {
        juce::ScopedJuceInitialiser_GUI initialise;
        require(argc == 2 || argc == 3, "usage: vdx7_state_transition_tests <private compatible ROM> [--save-only|--pending-only|--routing-only|--bank-only]");
        const juce::File rom(argv[1]);
        require(rom.existsAsFile(), "private test ROM exists");
        const auto mode = argc == 3 ? juce::String(argv[2]) : juce::String();
        require(mode.isEmpty() || mode == "--save-only" || mode == "--pending-only"
                || mode == "--routing-only" || mode == "--bank-only", "valid test selection");
        if (mode.isEmpty() || mode == "--bank-only") testFactoryVoiceValidation(rom);
        if (mode.isEmpty() || mode == "--save-only") testNoRomSave(rom);
        if (mode.isEmpty() || mode == "--pending-only")
        {
            testFreshFirstRom(rom);
            testPendingRestore(rom);
        }
        if (mode.isEmpty() || mode == "--routing-only") testLateMailboxPublication(rom);
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
