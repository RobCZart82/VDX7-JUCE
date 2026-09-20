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
    static void checkSerialOverflow(VDX7Engine& e)
    {
        e.dx7_.midiSerialRx.flush();
        for (int i = 0; i < 8190; ++i) e.dx7_.midiSerialRx.write(0xf8);
        const uint8_t note[] {0x90, 60, 100};
        e.handleMidi(note, 3);
        require(!e.hasHeldMidiNotes(), "serial overflow must reconcile note ownership");
        const int pending = (e.dx7_.midiSerialRx.writeIdx - e.dx7_.midiSerialRx.readIdx) & 8191;
        require(pending >= 384 && pending < 8191, "serial overflow must enqueue complete recovery messages");
        for (int pitch = 0; pitch < 128; ++pitch)
        {
            uint8_t status = 0, noteNumber = 0, velocity = 1;
            require(e.dx7_.midiSerialRx.read(status) && e.dx7_.midiSerialRx.read(noteNumber)
                    && e.dx7_.midiSerialRx.read(velocity), "complete recovery note-off");
            require((status & 0xf0) == 0x80 && noteNumber == pitch && velocity == 0,
                    "recovery releases every MIDI pitch");
        }
        e.handleMidi(note, 3);
        require(!e.hasHeldMidiNotes(), "same-timestamp events stay dropped during recovery");
        std::array<float, 256> left {}, right {};
        e.render(left.data(), right.data(), 256);
        require(!e.isMidiRecovering() && e.midiOverloadCount() == 1, "serial recovery completes");
        dx7Emu::Message ignored;
        while (e.toSynth_->pop(ignored)) {}
        e.processQueuedMessage({dx7Emu::Message::CtrlID::modulate, 7});
        for (int i = 0; i < 10000 && !e.dx7_.byte1Sent; ++i) e.dx7_.run();
        require(e.dx7_.byte1Sent && e.dx7_.haveMsg, "reach real in-flight controller handshake");
        for (int i = 0; i < 1024; ++i) e.toSynth_->analog(dx7Emu::Message::CtrlID::modulate, 127);
        const uint8_t pedal[] {0xb0, 64, 127};
        e.handleMidi(pedal, 3);
        require(e.isMidiRecovering() && e.midiOverloadCount() == 2 && !e.hasHeldMidiNotes(),
                "controller saturation must not lose pedal reconciliation");
        require((e.dx7_.P_CRT_PEDALS_LCD & 3) == 0, "recovery releases sustain and portamento");
        require(e.dx7_.byte1Sent && e.dx7_.haveMsg && e.dx7_.msg.byte2 == 7,
                "overload must preserve in-flight controller handshake");
        e.render(left.data(), right.data(), 1);
        std::vector<uint8_t> bank;
        require(e.saveRam(bank), "capture bank for live SysEx regression");
        bank.resize(4096);
        const auto sysex = VDX7Sysex::encode(bank);
        const int beforeBank = (e.dx7_.midiSerialRx.writeIdx - e.dx7_.midiSerialRx.readIdx) & 8191;
        require(e.handleSysex(sysex.data(), sysex.size()), "live bank after recovery starts draining");
        const int afterBank = (e.dx7_.midiSerialRx.writeIdx - e.dx7_.midiSerialRx.readIdx) & 8191;
        require(beforeBank >= 384 && afterBank == beforeBank + 2,
                "live bank must preserve queued recovery note-offs");
        e.resetMidiLifecycle();
        require(!e.isMidiRecovering(), "restart during recovery reconciles lifecycle");
        e.handleMidi(note, 3);
        require(e.hasHeldMidiNotes(), "fresh note accepted after recovery");
        e.allNotesOff();
    }
    static void publishWithoutEditor(VDX7AudioProcessor& p) { p.timerCallback(); }
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
    static void checkProgramBytes(VDX7Engine& e)
    {
        for (int program : {0, 31, 32, 127})
        {
            e.dx7_.midiSerialRx.flush();
            const uint8_t message[] {0xc0, static_cast<uint8_t>(program)};
            e.handleMidi(message, 2);
            uint8_t status = 0, value = 0;
            require(e.dx7_.midiSerialRx.read(status) && e.dx7_.midiSerialRx.read(value), "firmware program bytes");
            require((status & 0xf0) == 0xc0 && value == std::min(program, 31)
                    && e.currentProgram() == value, "program metadata and firmware input agree");
        }
    }
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

static void checkEditOrdering(const juce::File& romFile)
{
    for (bool bankSwitch : {false, true})
        for (bool editFirst : {false, true})
            for (bool audioFlush : {false, true})
            for (bool operatorEdit : {false, true})
            {
                VDX7AudioProcessor p(false);
                require(p.loadRomFromFile(romFile), "ordering ROM");
                p.selectProgramFromUi(3);
                const auto before = decode(save(p));
                juce::MemoryBlock originalRam;
                require(originalRam.fromBase64Encoding(before["ram"].toString()), "ordering initial RAM");
                auto* feedback = p.parameters().getParameter(operatorEdit
                    ? VDX7ParameterIDs::operatorParameter(5, VDX7VoiceData::Parameter::outputLevel)
                    : VDX7ParameterIDs::voiceParameter(VDX7VoiceData::VoiceParameter::feedback));
                const int editValue = (juce::roundToInt(feedback->convertFrom0to1(feedback->getValue())) + 1)
                    % (operatorEdit ? 100 : 8);
                auto patchExpected = [&](juce::MemoryBlock& ram, int program) {
                    auto* voice = static_cast<uint8_t*>(ram.getData()) + program*128;
                    if (operatorEdit)
                        VDX7VoiceData::setOperatorParameter(voice, 128, 5,
                            VDX7VoiceData::Parameter::outputLevel, editValue);
                    else VDX7VoiceData::setVoiceParameter(voice, 128,
                            VDX7VoiceData::VoiceParameter::feedback, editValue);
                };
                auto edit = [&] { feedback->setValueNotifyingHost(feedback->convertTo0to1(editValue)); };
                auto change = [&] {
                    if (bankSwitch) require(p.selectFactoryBank(1), "ordering bank selection");
                    else p.selectProgramFromUi(4);
                };
                if (editFirst) { edit(); change(); } else { change(); edit(); }
                if (audioFlush)
                {
                    juce::AudioBuffer<float> audio(2, 64);
                    juce::MidiBuffer midi;
                    p.processBlock(audio, midi);
                }
                juce::MemoryBlock actual;
                require(actual.fromBase64Encoding(decode(save(p))["ram"].toString()), "ordering final RAM");
                if (bankSwitch)
                {
                    VDX7AudioProcessor reference(false);
                    require(reference.loadRomFromFile(romFile), "reference ROM");
                    reference.selectProgramFromUi(3);
                    reference.selectFactoryBank(1);
                    juce::MemoryBlock expected;
                    require(expected.fromBase64Encoding(decode(save(reference))["ram"].toString()), "bank reference RAM");
                    // Factory-bank loading replaces the single internal RAM bank:
                    // an earlier edit must not leak into the newly loaded bank.
                    if (!editFirst)
                        patchExpected(expected, 3);
                    require(std::memcmp(actual.getData(), expected.getData(), 4096) == 0,
                            "bank switch/edit ordering preserves destination bank");
                }
                else
                {
                    patchExpected(originalRam, editFirst ? 3 : 4);
                    require(std::memcmp(actual.getData(), originalRam.getData(), 4096) == 0,
                            "program switch/edit ordering preserves correct voice");
                }
            }

    VDX7AudioProcessor rapid(false);
    require(rapid.loadRomFromFile(romFile), "rapid selection ROM");
    juce::MemoryBlock expected;
    require(expected.fromBase64Encoding(decode(save(rapid))["ram"].toString()), "rapid original RAM");
    auto* parameter = rapid.parameters().getParameter(VDX7ParameterIDs::voiceParameter(
        VDX7VoiceData::VoiceParameter::feedback));
    int value = juce::roundToInt(parameter->convertFrom0to1(parameter->getValue()));
    for (int program = 0; program < 32; ++program)
    {
        rapid.setCurrentProgram(program);
        value = (value + 1) % 8;
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
        VDX7VoiceData::setVoiceParameter(static_cast<uint8_t*>(expected.getData()) + program*128,
            128, VDX7VoiceData::VoiceParameter::feedback, value);
    }
    juce::MemoryBlock actual;
    require(actual.fromBase64Encoding(decode(save(rapid))["ram"].toString()), "rapid final RAM");
    require(std::memcmp(expected.getData(), actual.getData(), 4096) == 0,
            "32 successive program/edit pairs are not coalesced or misdirected");
}

static void checkAudioPublication(const juce::File& romFile)
{
    VDX7AudioProcessor p(false);
    require(p.loadRomFromFile(romFile), "publication ROM");
    struct Listener final : juce::AudioProcessorParameter::Listener
    {
        int calls = 0;
        VDX7AudioProcessor* reenter = nullptr;
        bool saved = false;
        bool coherent = true;
        juce::RangedAudioParameter* editOnNotify = nullptr;
        float editValue = 0;
        bool edited = false;
        void parameterValueChanged(int index, float) override
        {
            ++calls;
            if (editOnNotify != nullptr && !edited && index != editOnNotify->getParameterIndex())
            {
                edited = true;
                editOnNotify->setValueNotifyingHost(editValue);
            }
            if (reenter != nullptr)
            {
                juce::MemoryBlock state;
                reenter->getStateInformation(state);
                saved = state.getSize() > 0;
                const auto decoded = decode(state);
                juce::MemoryBlock ram;
                coherent &= ram.fromBase64Encoding(decoded["ram"].toString());
                if (ram.getSize() == VDX7Engine::kRamStateSize)
                {
                    const auto* voice = static_cast<const uint8_t*>(ram.getData())
                        + static_cast<int>(decoded["program"])*128;
                    for (int n = 0; n < VDX7VoiceData::kVoiceParameterCount; ++n)
                    {
                        const auto field = static_cast<VDX7VoiceData::VoiceParameter>(n);
                        const auto child = decoded.getChildWithName("PARAMETERS").getChildWithProperty(
                            "id", VDX7ParameterIDs::voiceParameter(field));
                        coherent &= static_cast<int>(child["value"]) ==
                            VDX7VoiceData::getVoiceParameter(voice, 128, field);
                    }
                }
                else coherent = false;
            }
        }
        void parameterGestureChanged(int, bool) override {}
    } listener;
    for (auto* parameter : p.getParameters()) parameter->addListener(&listener);
    juce::AudioBuffer<float> audio(2, 64);
    juce::MidiBuffer events;
    int audioCalls = 0;
    bool nonRealtimePublication = true;
    for (int kind = 0; kind < 3; ++kind)
    {
        listener.reenter = nullptr;
        events.clear();
        if (kind == 0) events.addEvent(juce::MidiMessage::programChange(1, 4), 0);
        else if (kind == 1) events.addEvent(juce::MidiMessage::controllerEvent(1, 32, 1), 0);
        else
        {
            juce::MemoryBlock ram;
            require(ram.fromBase64Encoding(decode(save(p))["ram"].toString()), "publication SysEx RAM");
            auto* bytes = static_cast<uint8_t*>(ram.getData());
            auto* voice = bytes + p.getCurrentProgram()*128;
            const auto field = VDX7VoiceData::VoiceParameter::algorithm;
            VDX7VoiceData::setVoiceParameter(voice, 128, field,
                (VDX7VoiceData::getVoiceParameter(voice, 128, field) + 1) % 32);
            auto sysex = VDX7Sysex::encode(std::vector<uint8_t>(bytes, bytes + 4096));
            events.addEvent(sysex.data(), static_cast<int>(sysex.size()), 0);
        }
        listener.calls = 0;
        listener.saved = false;
        p.processBlock(audio, events);
        audioCalls += listener.calls;
        listener.reenter = &p;
        if (kind == 1)
        {
            listener.editOnNotify = p.parameters().getParameter(VDX7ParameterIDs::voiceParameter(
                VDX7VoiceData::VoiceParameter::algorithm));
            const int oldValue = juce::roundToInt(listener.editOnNotify->convertFrom0to1(
                listener.editOnNotify->getValue()));
            listener.editValue = listener.editOnNotify->convertTo0to1((oldValue + 1) % 32);
        }
        VDX7RegressionAccess::publishWithoutEditor(p);
        VDX7RegressionAccess::publishWithoutEditor(p); // Finish any reentrant edit's newer snapshot.
        nonRealtimePublication &= listener.calls > 0 && listener.saved;
        if (kind == 1)
            nonRealtimePublication &= listener.edited
                && listener.editOnNotify->getValue() == listener.editValue;
        listener.editOnNotify = nullptr;
    }
    for (auto* parameter : p.getParameters()) parameter->removeListener(&listener);
    require(audioCalls == 0, "audio callback must not publish host parameters");
    require(nonRealtimePublication, "editorless publisher permits reentrant host state request");
    require(listener.coherent, "reentrant saves contain voice parameters matching packed RAM");
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
        checkAudioPublication(romFile);
        checkEditOrdering(romFile);
        VDX7Engine engine;
        require(engine.loadRomImage(static_cast<const uint8_t*>(rom.getData()), rom.getSize()), "engine ROM");
        VDX7RegressionAccess::checkControllers(engine);
        VDX7RegressionAccess::checkProgramBytes(engine);
        VDX7RegressionAccess::checkSerialOverflow(engine);
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
        events.addEvent(juce::MidiMessage::noteOn(1, 59, uint8_t(100)), 0);
        events.addEvent(juce::MidiMessage::noteOff(1, 59), 192);
        VDX7RegressionAccess::contend(restarted, events);
        juce::AudioBuffer<float> shortAudio(2, 64);
        events.clear();
        restarted.processBlock(shortAudio, events);
        require(VDX7RegressionAccess::noteActive(restarted, 59), "deferred note duration must not collapse");
        for (int i = 0; i < 3; ++i) restarted.processBlock(shortAudio, events);
        require(!VDX7RegressionAccess::noteActive(restarted, 59), "deferred note-off retains its later position");
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

        events.addEvent(juce::MidiMessage::noteOn(1, 60, uint8_t(100)), 0);
        restarted.processBlock(audio, events);
        events.addEvent(juce::MidiMessage::noteOn(1, 61, uint8_t(100)), 64);
        VDX7RegressionAccess::contend(restarted, events);
        const auto beforeRestart = decode(save(restarted));
        restarted.releaseResources();
        restarted.prepareToPlay(96000, 64);
        require(!VDX7RegressionAccess::noteActive(restarted, 60), "device restart clears old note ownership");
        events.clear();
        for (int i = 0; i < 16; ++i) restarted.processBlock(shortAudio, events);
        require(!VDX7RegressionAccess::noteActive(restarted, 61), "device restart discards deferred note-on");
        require(shortAudio.getMagnitude(0, 64) < 0.00001f, "device restart silences previous voices");
        const auto afterRestart = decode(save(restarted));
        juce::MemoryBlock ramBeforeRestart, ramAfterRestart;
        require(ramBeforeRestart.fromBase64Encoding(beforeRestart["ram"].toString())
                && ramAfterRestart.fromBase64Encoding(afterRestart["ram"].toString()), "restart RAM decode");
        require(std::memcmp(ramBeforeRestart.getData(), ramAfterRestart.getData(), 4096) == 0
                && beforeRestart["bank"] == afterRestart["bank"]
                && beforeRestart["program"] == afterRestart["program"], "restart preserves bank/program/voices");

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
        VDX7RegressionAccess::publishWithoutEditor(restarted);
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
