#pragma once

#include "VDX7StatusPresentation.h"

// Private ROM fixture only: never copied to source/artifacts. No editor or host.
static void checkPendingOperationBoundary(const juce::File& romFile)
{
    juce::MemoryBlock rom;
    require(romFile.loadFileAsData(rom) && rom.getSize() == 49152, "pending fixture: combined ROM");
    static_cast<uint8_t*>(rom.getData())[16384 + 118] ^= 1; // Factory name, not firmware code.
    juce::TemporaryFile alternate;
    require(alternate.getFile().replaceWithData(rom.getData(), rom.getSize()), "private alternate ROM");
    const auto feedbackId = VDX7ParameterIDs::voiceParameter(VDX7VoiceData::VoiceParameter::feedback);
    const auto opId = VDX7ParameterIDs::operatorParameter(5, VDX7VoiceData::Parameter::outputLevel);
    auto set = [](VDX7AudioProcessor& p, const juce::String& id, int value) {
        auto* parameter = p.parameters().getParameter(id);
        require(parameter != nullptr, "pending parameter exists");
        parameter->setValueNotifyingHost(parameter->convertTo0to1(float(value)));
    };
    auto source = std::make_unique<VDX7AudioProcessor>(false);
    require(source->loadRomFromFile(romFile), "pending source ROM");
    source->setCurrentProgram(7);
    set(*source, feedbackId, 1);
    auto tree = decode(save(*source));
    tree.setProperty("romPath", alternate.getFile().getSiblingFile("missing-pending-test.bin").getFullPathName(), nullptr);
    const auto state = encode(tree);
    juce::String error;
    VDX7AudioProcessor::SyxExportSnapshot single, bank;
    require(source->captureSyxExportSnapshot(false, single, error), "ready single export");
    require(source->captureSyxExportSnapshot(true, bank, error), "ready bank export");
    juce::TemporaryFile singleFile, bankFile;
    require(singleFile.getFile().replaceWithData(single.message().data(), single.message().size()), "single fixture");
    require(bankFile.getFile().replaceWithData(bank.message().data(), bank.message().size()), "bank fixture");
    juce::TemporaryFile userFile;
    VDX7UserBank::Voice userVoice{};
    require(source->captureUserPatch(userVoice, error), "ready USER fixture");
    VDX7UserBank::Snapshot emptyUser;
    require(VDX7UserBank::savePatch(userFile.getFile(), emptyUser, 0, userVoice, "PENDING", false).wasOk(), "private USER fixture");

    for (bool loaded : {false, true})
    for (bool callbacks : {false, true})
    for (bool reopen : {false, true})
    {
        auto p = std::make_unique<VDX7AudioProcessor>(false);
        if (loaded)
        {
            require(p->loadRomFromFile(alternate.getFile()), "foreign ROM");
            require(p->copyOperator(0), "ready clipboard control");
        }
        p->prepareToPlay(48000, 64);
        p->setStateInformation(state.getData(), int(state.getSize()));
        set(*p, feedbackId, 6);
        set(*p, opId, 42);
        const int programBefore = p->getCurrentProgram(), bankBefore = p->getCurrentBank();
        require(!p->loadSyxFromFile(singleFile.getFile(), &error), "pending single import rejected");
        require(!p->loadSyxFromFile(bankFile.getFile(), &error), "pending bank import rejected");
        require(!p->loadUserBank(userFile.getFile(), error), "pending USER import rejected");
        VDX7UserBank::Voice voice{};
        VDX7AudioProcessor::SyxExportSnapshot snapshot;
        require(!p->captureUserPatch(voice, error), "pending USER capture rejected");
        require(!p->captureSyxExportSnapshot(false, snapshot, error), "pending single export rejected");
        require(!p->captureSyxExportSnapshot(true, snapshot, error), "pending bank export rejected");
        require(!p->renameVoice("NO CHANGE") && !p->copyOperator(1) && !p->pasteOperator(2), "pending edit APIs rejected");
        require(!p->setMasterTuneFromUi(100), "pending tune rejected");
        for (int field = 0; field < 4; ++field)
            require(!p->setPlaySettingFromUi(field, 1), "pending play rejected");
        for (int field = 0; field < 2; ++field)
            require(!p->setPitchBendSettingFromUi(field, 12), "pending bend rejected");
        require(!p->setControllerSettingFromUi(0, 0, 99), "pending controller rejected");
        require(!p->selectFactoryBank(2), "pending bank rejected");
        p->setCurrentProgram(12);
        require(p->getCurrentProgram() == programBefore && p->getCurrentBank() == bankBefore, "pending selection unchanged");
        if (callbacks)
        {
            juce::AudioBuffer<float> audio(2, 64);
            juce::MidiBuffer midi;
            midi.addEvent(juce::MidiMessage(single.message().data(), int(single.message().size())), 0);
            midi.addEvent(juce::MidiMessage::programChange(1, 12), 1);
            midi.addEvent(juce::MidiMessage::controllerEvent(1, 32, 2), 2);
            midi.addEvent(juce::MidiMessage::controllerEvent(1, 5, 99), 3);
            p->processBlock(audio, midi);
            require(p->getCurrentProgram() == programBefore && p->getCurrentBank() == bankBefore, "pending live selection unchanged");
        }
        const auto status = VDX7StatusPresentation::choose(p->getCriticalStatusText(), p->hasUnexportedEdits(), p->getStatusText());
        require(status.containsIgnoreCase("project") && status.containsIgnoreCase("ROM")
                && !status.contains("SAVE AS"), "pending recovery warning outranks dirty export advice");
        if (reopen)
        {
            const auto checkpoint = save(*p);
            p->setStateInformation(checkpoint.getData(), int(checkpoint.getSize()));
        }
        require(p->loadRomFromFile(romFile), "matching ROM restores project");
        require(p->parameters().getRawParameterValue(feedbackId)->load() == 6
                && p->parameters().getRawParameterValue(opId)->load() == 42, "pending edits survive rejected operations");
        require(p->getCurrentProgram() == 7, "saved program restored");
        require(p->getMasterTune() == source->getMasterTune()
                && p->getPitchBendSettings() == source->getPitchBendSettings(), "rejected performance does not leak into restore");
        require(p->captureUserPatch(voice, error) && p->captureSyxExportSnapshot(false, snapshot, error), "ready capture restored");
        require(p->loadSyxFromFile(singleFile.getFile(), &error), "ready import restored");
        require(p->setMasterTuneFromUi(100) && p->setPitchBendSettingFromUi(0, 12), "ready performance restored");
    }
}
