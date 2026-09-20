#include "PluginProcessor.h"
#include "VDX7Sysex.h"
#include <cmath>
#include <future>
#include <iostream>
#include <stdexcept>
#include <thread>

static void require(bool ok, const char* message)
{ if (!ok) throw std::runtime_error(message); }

struct VDX7RegressionAccess
{
    static bool invalidRom(VDX7AudioProcessor& p)
    { return p.loadRomData(juce::File(), std::vector<uint8_t>(7), nullptr); }
    static void checkControllers(VDX7Engine& e)
    {
        for (int cc : {64, 65}) for (int v : {0, 1, 63, 64, 127})
        {
            uint8_t event[] {0xb0, static_cast<uint8_t>(cc), static_cast<uint8_t>(v)};
            e.handleMidi(event, 3);
            dx7Emu::Message m;
            while (e.toSynth_->pop(m)) e.processQueuedMessage(m);
            require(bool(e.dx7_.P_CRT_PEDALS_LCD & (cc == 64 ? 1 : 2)) == (v >= 64),
                    "CC switch threshold");
        }
        for (int volume : {1, 7}) for (int expression : {0, 64, 127})
        {
            uint8_t event[] {0xb0, 11, static_cast<uint8_t>(expression)};
            e.handleMidi(event, 3);
            e.dx7_.midiVolume = static_cast<uint8_t>(volume);
            float result = 0;
            for (int i = 0; i < 30000; ++i)
            {
                e.nativePos_ = 0; e.nativeCount_ = 1; e.nativeBlock_[0] = 1;
                result = e.nextNativeSample();
            }
            const float expected = e.dx7_.midiVolTab[volume] * expression / 127.0f;
            require(std::abs(result - expected) < 0.0001f, "Expression gain including silence");
        }
        e.resetAudioState();
    }
    static void contend(VDX7AudioProcessor& p, juce::MidiBuffer& events)
    {
        std::promise<void> locked, release;
        auto untilRelease = release.get_future();
        auto worker = std::thread([&]
        {
            std::scoped_lock lock(p.engineMutex_);
            locked.set_value();
            untilRelease.wait();
        });
        locked.get_future().wait();
        juce::AudioBuffer<float> audio(2, 256);
        p.processBlock(audio, events);
        release.set_value();
        worker.join();
    }
    static bool noteActive(VDX7AudioProcessor& p, int note)
    { return p.engine_.activeMidiNotes_[note]; }
};

static juce::MemoryBlock save(VDX7AudioProcessor& p)
{ juce::MemoryBlock s; p.getStateInformation(s); return s; }
static juce::ValueTree decode(const juce::MemoryBlock& s)
{
    auto xml = juce::AudioProcessor::getXmlFromBinary(s.getData(), static_cast<int>(s.getSize()));
    require(xml != nullptr, "state XML");
    return juce::ValueTree::fromXml(*xml);
}
static juce::MemoryBlock encode(const juce::ValueTree& s)
{
    juce::MemoryBlock result;
    auto xml = s.createXml();
    juce::AudioProcessor::copyXmlToBinary(*xml, result);
    return result;
}

int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    if (argc != 2) { std::cout << "SKIP: supply your own ROM path\n"; return 77; }
    try
    {
        juce::File romFile(juce::String::fromUTF8(argv[1]));
        juce::MemoryBlock rom;
        require(romFile.loadFileAsData(rom), "read local ROM");
        VDX7Engine engine;
        require(engine.loadRomImage(static_cast<const uint8_t*>(rom.getData()), rom.getSize()), "engine ROM");
        VDX7RegressionAccess::checkControllers(engine);
        std::vector<uint8_t> before, after;
        engine.saveRam(before);
        const uint8_t invalid[7] {};
        require(!engine.loadRomImage(invalid, sizeof(invalid)), "reject bad ROM");
        require(engine.isLoaded() && engine.hasFactoryVoices(), "bad ROM preserves running engine");
        engine.saveRam(after);
        require(before == after, "bad ROM preserves RAM");
        require(engine.loadRomImage(static_cast<const uint8_t*>(rom.getData()), 16384), "firmware-only reload");
        require(!engine.hasFactoryVoices() && engine.currentBank() == -1, "no stale factory bank");

        // Temporary local firmware is removed by the directory guard,
        // including when the assertion throws. Never part of an artifact.
        const auto temporaryDirectory = juce::File::getSpecialLocation(juce::File::tempDirectory)
            .getNonexistentChildFile("vdx7-companion-regression", "", false);
        require(temporaryDirectory.createDirectory().wasOk(), "create private test directory");
        struct RemoveDirectory
        {
            juce::File directory;
            ~RemoveDirectory() { directory.deleteRecursively(); }
        } cleanup { temporaryDirectory };
        const auto firmwareFile = temporaryDirectory.getChildFile("firmware.bin");
        require(firmwareFile.replaceWithData(rom.getData(), 16384), "write temporary firmware");
        require(temporaryDirectory.getChildFile("dx7_factory_voices_32KB.bin")
                    .replaceWithData(invalid, sizeof(invalid)), "write invalid companion");
        VDX7AudioProcessor companion(false);
        require(companion.loadRomFromFile(firmwareFile), "invalid optional companion permits firmware load");
        require(companion.isRomLoaded() && !companion.hasFactoryVoices(), "firmware-only companion fallback");
        require(companion.getStatusText().containsIgnoreCase("ignored"), "companion warning status");
        const auto companionFile = temporaryDirectory.getChildFile("dx7_factory_voices_32KB.bin");
        require(companionFile.replaceWithData(static_cast<const uint8_t*>(rom.getData()) + 16384,
                                             32768), "write valid temporary companion");
        require(companion.loadRomFromFile(firmwareFile) && companion.hasFactoryVoices(),
                "valid companion loads factory banks");
        require(companionFile.deleteFile(), "remove temporary companion");
        require(companion.loadRomFromFile(firmwareFile) && !companion.hasFactoryVoices(),
                "absent companion loads firmware only");

        VDX7AudioProcessor original(false);
        require(original.loadRomFromFile(romFile), "processor ROM");
        original.selectProgramFromUi(5);
        save(original);
        require(original.renameVoice("RESTORE1"), "edit name");
        auto* feedback = original.parameters().getParameter(VDX7ParameterIDs::voiceParameter(
            VDX7VoiceData::VoiceParameter::feedback));
        feedback->setValueNotifyingHost(feedback->convertTo0to1(6));
        require(!VDX7RegressionAccess::invalidRom(original), "processor rejects invalid ROM");
        require(original.parameters().getRawParameterValue(VDX7ParameterIDs::voiceParameter(
            VDX7VoiceData::VoiceParameter::feedback))->load() == 6, "invalid ROM preserves pending UI edit");
        auto saved = decode(save(original));
        saved.setProperty("romPath", "/vdx7-regression-missing/firmware.bin", nullptr);
        auto missing = encode(saved);
        VDX7AudioProcessor waiting(false);
        waiting.setStateInformation(missing.getData(), static_cast<int>(missing.getSize()));
        require(!waiting.isRomLoaded(), "missing ROM remains unloaded");
        require(decode(save(waiting)).isEquivalentTo(saved), "save preserves pending state");
        auto* master = waiting.parameters().getParameter("masterVolume");
        require(master != nullptr, "master parameter exists");
        master->setValueNotifyingHost(0.37f);
        const float editedMasterValue = master->getValue();
        const auto editedPending = decode(save(waiting));
        require(!editedPending.isEquivalentTo(saved), "missing-ROM re-save includes new host parameter");
        require(editedPending["ram"] == saved["ram"]
                && editedPending["bank"] == saved["bank"]
                && editedPending["program"] == saved["program"], "missing-ROM edit preserves sound data");
        VDX7AudioProcessor restarted(false);
        auto pending = save(waiting);
        restarted.setStateInformation(pending.getData(), static_cast<int>(pending.getSize()));
        require(std::abs(restarted.parameters().getParameter("masterVolume")->getValue() - editedMasterValue)
                < 0.00001f, "missing-ROM master edit survives second restore");
        require(restarted.loadRomFromFile(romFile), "manual ROM resumes restore");
        require(std::abs(restarted.parameters().getParameter("masterVolume")->getValue() - editedMasterValue)
                < 0.00001f, "master edit survives firmware load");
        auto restored = decode(save(restarted));
        require(restored["ram"] == saved["ram"] && restored["program"] == saved["program"]
                && restored["bank"] == saved["bank"], "exact deferred RAM/bank/program restore");
        require(restarted.getCurrentPatchName() == "RESTORE1", "restored name");
        for (bool saveBeforeLoad : {false, true})
        {
            VDX7AudioProcessor edited(false);
            edited.setStateInformation(missing.getData(), static_cast<int>(missing.getSize()));
            auto* editedFeedback = edited.parameters().getParameter(VDX7ParameterIDs::voiceParameter(
                VDX7VoiceData::VoiceParameter::feedback));
            editedFeedback->setValueNotifyingHost(editedFeedback->convertTo0to1(2));
            auto* editedOperator = edited.parameters().getParameter(VDX7ParameterIDs::operatorParameter(
                5, VDX7VoiceData::Parameter::outputLevel));
            editedOperator->setValueNotifyingHost(editedOperator->convertTo0to1(42));
            if (saveBeforeLoad)
            {
                const auto checkpoint = save(edited);
                edited.setStateInformation(checkpoint.getData(), static_cast<int>(checkpoint.getSize()));
            }
            require(edited.loadRomFromFile(romFile), "load after offline voice edit");
            require(editedFeedback->getValue() == editedFeedback->convertTo0to1(2),
                    "explicit offline voice edit overrides saved RAM");
            require(editedOperator->getValue() == editedOperator->convertTo0to1(42),
                    "offline operator edit in upper dirty mask survives restore");
            require(edited.getCurrentPatchName() == "RESTORE1", "offline edit preserves remaining voice data");
        }
        auto malformed = saved.createCopy();
        malformed.setProperty("ram", "invalid-base64", nullptr);
        auto malformedBytes = encode(malformed);
        restarted.setStateInformation(malformedBytes.getData(), static_cast<int>(malformedBytes.getSize()));
        require(decode(save(restarted))["ram"] == saved["ram"], "malformed state preserves sound");
        for (auto* parameter : original.getParameters())
            if (parameter != original.parameters().getParameter("masterVolume"))
                require(std::abs(parameter->getValue() - restarted.getParameters()[parameter->getParameterIndex()]->getValue())
                        < 0.00001f, "restored host parameter");

        restarted.prepareToPlay(48000, 256);
        juce::AudioBuffer<float> audio(2, 256);
        juce::MidiBuffer events;
        events.addEvent(juce::MidiMessage::noteOn(1, 60, uint8_t(100)), 0);
        restarted.processBlock(audio, events);
        require(VDX7RegressionAccess::noteActive(restarted, 60), "note starts");
        events.clear();
        events.addEvent(juce::MidiMessage::noteOff(1, 60), 0);
        events.addEvent(juce::MidiMessage::controllerEvent(1, 64, 0), 1);
        VDX7RegressionAccess::contend(restarted, events);
        require(VDX7RegressionAccess::noteActive(restarted, 60), "busy engine defers note-off");
        events.clear(); restarted.processBlock(audio, events);
        require(!VDX7RegressionAccess::noteActive(restarted, 60), "deferred note-off delivered");

        events.addEvent(juce::MidiMessage::noteOn(1, 61, uint8_t(100)), 0);
        restarted.processBlock(audio, events);
        events.clear();
        for (int i = 0; i < 257; ++i)
            events.addEvent(juce::MidiMessage::noteOn(1, 62, uint8_t(100)), 0);
        VDX7RegressionAccess::contend(restarted, events);
        events.clear(); restarted.processBlock(audio, events);
        require(!VDX7RegressionAccess::noteActive(restarted, 61)
                && !VDX7RegressionAccess::noteActive(restarted, 62), "overflow releases without replaying stale ons");

        juce::MemoryBlock ram;
        require(ram.fromBase64Encoding(restored["ram"].toString()), "decode RAM for bank");
        const auto* start = static_cast<const uint8_t*>(ram.getData());
        std::vector<uint8_t> packed(start, start + 4096);
        packed[5 * 128 + 118] = 'L';
        VDX7VoiceData::setVoiceParameter(packed.data()+5*128, 128,
            VDX7VoiceData::VoiceParameter::algorithm, 31);
        auto sysex = VDX7Sysex::encode(packed);
        require(sysex.size() == 4104, "encode bank");
        events.addEvent(sysex.data(), static_cast<int>(sysex.size()), 0);
        restarted.processBlock(audio, events);
        require(restarted.getCurrentBank() == -1 && restarted.hasUnexportedEdits(), "live bank metadata");
        require(restarted.getCurrentPatchName().startsWith("L"), "live bank name refreshed");
        require(restarted.parameters().getRawParameterValue(VDX7ParameterIDs::voiceParameter(
            VDX7VoiceData::VoiceParameter::algorithm))->load() == 31, "live bank host parameter refreshed");
        sysex[6] |= 0x80;
        events.clear(); events.addEvent(sysex.data(), static_cast<int>(sysex.size()), 0);
        restarted.processBlock(audio, events);
        require(restarted.getCurrentPatchName().startsWith("L"), "invalid live bank ignored");
        events.clear();
        events.addEvent(juce::MidiMessage::controllerEvent(16, 32, 2), 0);
        restarted.processBlock(audio, events);
        require(restarted.getCurrentBank() == 2, "CC32 bank selection");
        juce::MemoryBlock changedRam;
        require(changedRam.fromBase64Encoding(decode(save(restarted))["ram"].toString()), "bank RAM");
        require(std::memcmp(changedRam.getData(), static_cast<const uint8_t*>(rom.getData())
                + 16384 + 2*4096, 4096) == 0, "CC32 and editor use the same internal bank");
        std::cout << "PASS: pedal thresholds, expression, invalid ROM, deferred restore, contention and live bank state\n";
        return 0;
    }
    catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
