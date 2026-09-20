#include "PluginEditor.h"
#include "VDX7MechanicalDrawing.h"
#include <iostream>
#include <cmath>
#include <stdexcept>

static void require(bool ok, const char* message)
{
    if (!ok) throw std::runtime_error(message);
}

static float value(VDX7AudioProcessor& p, const juce::String& id)
{
    return p.parameters().getRawParameterValue(id)->load();
}

static void set(VDX7AudioProcessor& p, const juce::String& id, float v)
{
    auto* parameter = p.parameters().getParameter(id);
    require(parameter != nullptr, "missing host parameter");
    parameter->setValueNotifyingHost(parameter->convertTo0to1(v));
}

static juce::MemoryBlock save(VDX7AudioProcessor& p)
{
    juce::MemoryBlock data;
    p.getStateInformation(data);
    return data;
}

static juce::MemoryBlock ram(const juce::MemoryBlock& state)
{
    auto xml = juce::AudioProcessor::getXmlFromBinary(state.getData(), int(state.getSize()));
    require(xml != nullptr, "state XML");
    juce::MemoryBlock result;
    require(result.fromBase64Encoding(juce::ValueTree::fromXml(*xml)["ram"].toString()),
            "state RAM decoding");
    return result;
}

static void checkUserLibrary(const juce::File& romFile, const juce::File& imageFolder)
{
    juce::TemporaryFile temporary;
    const auto file = temporary.getFile();
    VDX7AudioProcessor p(false);
    VDX7UserBank::Voice captured {};
    juce::String error;
    require(!p.captureUserPatch(captured, error), "capture requires ROM");
    require(p.loadRomFromFile(romFile), "USER test ROM");
    require(p.renameVoice("CAPTURED"), "rename source");
    {
        std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());
        juce::TextButton* saveButton = nullptr;
        bool userItem = false;
        for (auto* child : editor->getChildren())
        {
            if (auto* button = dynamic_cast<juce::TextButton*>(child))
                if (button->getButtonText() == "SAVE AS...") saveButton = button;
            if (auto* box = dynamic_cast<juce::ComboBox*>(child))
                userItem |= box->getItemText(box->indexOfItemId(9)) == "USER (load copy)";
        }
        require(saveButton && saveButton->isEnabled() && userItem, "USER GUI entry points");
        require(juce::Desktop::getInstance().getDisplays().getPrimaryDisplay() != nullptr,
                "SAVE AS GUI test requires desktop/display access (not a headless sandbox)");
        saveButton->onClick();
        auto* dialog = dynamic_cast<juce::AlertWindow*>(juce::Component::getCurrentlyModalComponent());
        require(dialog != nullptr, "SAVE AS dialog opens");
        require(dialog->getComboBoxComponent("action")->getSelectedItemIndex() == 0, "USER is default save action");
        require(dialog->getComboBoxComponent("slot")->getNumItems() == 32, "32 destination slots");
        require(dialog->getTextEditorContents("name") == "CAPTURED", "captured patch name in dialog");
        if (imageFolder != juce::File())
        {
            juce::FileOutputStream stream(imageFolder.getChildFile("VDX7-save-as.png"));
            require(stream.openedOk(), "save dialog screenshot file");
            require(juce::PNGImageFormat().writeImageToStream(dialog->createComponentSnapshot(dialog->getLocalBounds()), stream), "save dialog screenshot");
        }
        dialog->exitModalState(0); // Cancel: callback has no writes; editor destruction is safe.
        juce::TextButton* settings = nullptr;
        for (auto* child : editor->getChildren())
            if (auto* button = dynamic_cast<juce::TextButton*>(child))
                if (button->getButtonText() == "SETTINGS") settings = button;
        require(settings && settings->isEnabled(), "SETTINGS is connected");
        settings->onClick();
        auto* tuning = dynamic_cast<juce::AlertWindow*>(juce::Component::getCurrentlyModalComponent());
        require(tuning && tuning->getTextEditorContents("tuning") == juce::String(p.getMasterTune()),
                "SETTINGS shows current firmware tuning");
        require(tuning->getComboBoxComponent("channel")->getNumItems() == 17
                && tuning->getComboBoxComponent("channel")->getSelectedItemIndex() == p.getMidiInputChannel(),
                "SETTINGS has OMNI plus 16 channels");
        if (imageFolder != juce::File())
        {
            juce::FileOutputStream stream(imageFolder.getChildFile("VDX7-settings.png"));
            require(stream.openedOk() && juce::PNGImageFormat().writeImageToStream(
                tuning->createComponentSnapshot(tuning->getLocalBounds()), stream), "settings screenshot");
        }
        tuning->exitModalState(0);
        juce::TextButton* about = nullptr;
        for (auto* child : editor->getChildren())
            if (auto* button = dynamic_cast<juce::TextButton*>(child))
                if (button->getButtonText() == "ABOUT") about = button;
        require(about != nullptr, "ABOUT button exists");
        about->onClick();
        auto* credits = dynamic_cast<juce::AlertWindow*>(juce::Component::getCurrentlyModalComponent());
        require(credits != nullptr, "ABOUT opens");
        bool foundLogo = false;
        for (auto* child : credits->getChildren())
            if (auto* logo = dynamic_cast<juce::ImageComponent*>(child))
                foundLogo |= logo->getName() == "GYR logo" && logo->getImage().isValid();
        require(foundLogo, "ABOUT embeds valid GYR logo");
        if (imageFolder != juce::File())
        {
            juce::FileOutputStream stream(imageFolder.getChildFile("VDX7-about.png"));
            require(stream.openedOk() && juce::PNGImageFormat().writeImageToStream(
                credits->createComponentSnapshot(credits->getLocalBounds()), stream), "about screenshot");
        }
        credits->exitModalState(0);
    }
    require(p.captureUserPatch(captured, error), "capture queued voice");
    const auto before = ram(save(p));
    VDX7UserBank::Snapshot expected;
    require(VDX7UserBank::load(file, expected).wasOk(), "empty library snapshot");
    require(VDX7UserBank::savePatch(file, expected, 7, captured, "USER ONE", false).wasOk(), "save USER copy");
    require(ram(save(p)) == before && p.hasUnexportedEdits(), "copy does not mutate working bank or dirty flag");
    require(p.renameVoice("LATER"), "edit after snapshot");
    require(VDX7UserBank::load(file, expected).wasOk(), "reopen library");
    require(VDX7UserBank::savePatch(file, expected, 12, captured, "USER TWO", false).wasOk(), "save immutable snapshot");
    require(p.setControllerSettingFromUi(0, 0, 42), "global before USER load");
    const auto controllers = p.getControllerSettings();
    require(p.loadUserBank(file, error), "USER load");
    require(p.getCurrentProgram() == 7 && p.getCurrentPatchName() == "USER ONE", "first occupied slot selected");
    require(p.getCurrentBank() == -1 && !p.hasUnexportedEdits(), "USER is independent clean CUSTOM copy");
    require(p.getControllerSettings() == controllers, "USER load preserves globals");
    VDX7UserBank::Voice loaded;
    require(p.captureUserPatch(loaded, error), "capture loaded USER");
    require(std::equal(captured.begin(), captured.begin() + 118, loaded.begin()), "saved voice bytes unchanged");
    auto state = save(p);
    VDX7AudioProcessor restored(false);
    require(restored.loadRomFromFile(romFile), "restore USER ROM");
    restored.setStateInformation(state.getData(), int(state.getSize()));
    require(restored.getCurrentPatchName() == "USER ONE", "USER copy restores in project");
    require(restored.getControllerSettings() == controllers, "USER project globals restore");
    juce::MemoryBlock diskBefore, diskAfter;
    require(file.loadFileAsData(diskBefore), "read library bytes");
    require(p.renameVoice("WORKING"), "edit working copy");
    require(file.loadFileAsData(diskAfter) && diskBefore == diskAfter, "working edits do not autosave USER file");
    require(file.replaceWithText("damaged"), "corrupt test library");
    auto preserved = ram(save(p));
    require(!p.loadUserBank(file, error) && ram(save(p)) == preserved, "damaged USER does not replace working bank");
}

static void checkControllers(const juce::File& romFile)
{
    VDX7AudioProcessor p(false);
    require(!p.setControllerSettingFromUi(0, 0, 50), "controller edit needs ROM");
    require(p.loadRomFromFile(romFile), "controller test ROM");
    require(!p.setMasterTuneFromUi(-257) && !p.setMasterTuneFromUi(256), "tuning bounds");
    const auto voicesBeforeTuning = ram(save(p));
    for (int value = -256; value <= 255; ++value)
    {
        require(p.setMasterTuneFromUi(value) && p.getMasterTune() == value, "all tuning values round-trip");
    }
    const auto tuned = ram(save(p));
    require(std::memcmp(voicesBeforeTuning.getData(), tuned.getData(), 4096) == 0
            && !p.hasUnexportedEdits(), "tuning preserves voice bank and dirty state");
    {
        require(p.setMidiInputChannelFromUi(16), "select stored MIDI channel");
        const auto state = save(p);
        VDX7AudioProcessor restored(false);
        require(!restored.setMasterTuneFromUi(1), "tuning requires ROM");
        require(restored.loadRomFromFile(romFile), "tuning restore ROM");
        restored.setStateInformation(state.getData(), int(state.getSize()));
        require(restored.getMasterTune() == 255, "tuning project recall");
        require(restored.getMidiInputChannel() == 16, "MIDI input channel project recall");
        auto legacy = juce::ValueTree::fromXml(*juce::AudioProcessor::getXmlFromBinary(
            state.getData(), int(state.getSize())));
        legacy.removeProperty("midiInputChannel", nullptr);
        juce::MemoryBlock legacyBytes;
        juce::AudioProcessor::copyXmlToBinary(*legacy.createXml(), legacyBytes);
        restored.setStateInformation(legacyBytes.getData(), int(legacyBytes.getSize()));
        require(restored.getMidiInputChannel() == 0, "legacy state restores OMNI");
    }
    p.setMidiInputChannelFromUi(0);
    require(p.setMasterTuneFromUi(0), "reset tuning");
    require(!p.setPlaySettingFromUi(-1, 0) && !p.setPlaySettingFromUi(4, 0)
            && !p.setPlaySettingFromUi(0, 2) && !p.setPlaySettingFromUi(3, 100), "play setting bounds");
    for (int field = 0; field < 4; ++field)
    {
        require(p.setPlaySettingFromUi(field, field == 3 ? 63 : 1), "play setting accepted");
        require(p.getPlaySettings()[field] == (field == 3 ? 63 : 1), "play setting immediate capture");
    }
    {
        const auto state = save(p);
        VDX7AudioProcessor restored(false);
        require(restored.loadRomFromFile(romFile), "play restore ROM");
        restored.setStateInformation(state.getData(), int(state.getSize()));
        require(restored.getPlaySettings() == p.getPlaySettings(), "play settings restore");
        require(!p.hasUnexportedEdits(), "play settings do not dirty voice bank");
    }
    require(p.setPlaySettingFromUi(0, 0), "return to poly");
    require(!p.setPitchBendSettingFromUi(-1, 2) && !p.setPitchBendSettingFromUi(2, 2)
            && !p.setPitchBendSettingFromUi(0, 13) && !p.setPitchBendSettingFromUi(1, -1), "bend validation");
    for (int field = 0; field < 2; ++field)
        for (int v : {0,12,3}) require(p.setPitchBendSettingFromUi(field, v), "bend settings");
    require(!p.hasUnexportedEdits(), "bend globals do not dirty voice");
    {
        auto state = save(p);
        VDX7AudioProcessor other(false);
        require(other.loadRomFromFile(romFile), "bend restore ROM");
        other.setStateInformation(state.getData(), int(state.getSize()));
        require(other.getPitchBendSettings() == p.getPitchBendSettings(), "bend project restore");
    }
    const auto initial = ram(save(p));
    constexpr int offsets[] { 0, 2, 4, 6 };
    for (int c = 0; c < 4; ++c)
    {
        for (int v : { 0, 99, 37 + c })
            require(p.setControllerSettingFromUi(c, 0, v), "valid controller range");
        for (int mask = 0; mask < 8; ++mask)
        {
            for (int f = 1; f <= 3; ++f)
                require(p.setControllerSettingFromUi(c, f, (mask >> (f - 1)) & 1), "assignment edit");
            auto data = ram(save(p));
            const auto* bytes = static_cast<const uint8_t*>(data.getData());
            require((bytes[0x132e + offsets[c]] & 7) == mask, "exact firmware assignment bits");
            require(bytes[0x1336 + offsets[c]] == 37 + c, "exact firmware range location");
        }
    }
    const auto settings = p.getControllerSettings();
    const auto state = save(p);
    const auto changed = ram(state);
    for (size_t i = 0; i < changed.getSize(); ++i)
    {
        bool controllerByte = false;
        for (int offset : offsets)
            controllerByte |= i == size_t(0x132e + offset) || i == size_t(0x1336 + offset);
        if (!controllerByte)
            require(static_cast<const uint8_t*>(initial.getData())[i]
                        == static_cast<const uint8_t*>(changed.getData())[i], "controller edit isolates RAM fields");
    }
    require(!p.hasUnexportedEdits(), "global controls do not dirty voice bank");
    require(!p.setControllerSettingFromUi(-1, 0, 0)
            && !p.setControllerSettingFromUi(4, 0, 0)
            && !p.setControllerSettingFromUi(0, 4, 0)
            && !p.setControllerSettingFromUi(0, -1, 0)
            && !p.setControllerSettingFromUi(0, 0, 100)
            && !p.setControllerSettingFromUi(0, 0, -1)
            && !p.setControllerSettingFromUi(0, 1, 2), "invalid settings rejected");
    require(ram(save(p)) == changed, "invalid edits preserve RAM");
    p.prepareToPlay(48000, 256);
    juce::AudioBuffer<float> audio(2, 256);
    juce::MidiBuffer midi;
    for (int block = 0; block < 30; ++block) p.processBlock(audio, midi);
    require(p.getControllerSettings() == settings, "firmware retains settings while running");
    p.selectFactoryBank(1);
    save(p);
    require(p.getControllerSettings() == settings, "factory selection preserves globals");
    juce::TemporaryFile exported;
    juce::String error;
    require(p.exportSyx(exported.getFile(), true, error), "controller test bank export");
    p.setControllerSettingFromUi(0, 0, 12);
    require(p.loadSyxFromFile(exported.getFile(), &error), "controller test bank import");
    require(p.getControllerSettings()[0] == 12, "bank SysEx does not restore performance globals");

    // Existing full-RAM project format carries settings, including missing-ROM resaves.
    auto xml = juce::AudioProcessor::getXmlFromBinary(state.getData(), int(state.getSize()));
    auto missingState = juce::ValueTree::fromXml(*xml);
    missingState.setProperty("romPath", "", nullptr);
    missingState.setProperty("midiInputChannel", 12, nullptr);
    juce::MemoryBlock missingData;
    juce::AudioProcessor::copyXmlToBinary(*missingState.createXml(), missingData);
    VDX7AudioProcessor missing(false);
    missing.setStateInformation(missingData.getData(), int(missingData.getSize()));
    require(!missing.isRomLoaded(), "deliberate missing ROM");
    require(missing.getMidiInputChannel() == 12, "missing-ROM channel recall");
    require(missing.setMidiInputChannelFromUi(9), "edit channel while ROM missing");
    require(!missing.setControllerSettingFromUi(0, 0, 1), "missing ROM preserves pending globals");
    const auto resaved = save(missing);
    VDX7AudioProcessor restored(false);
    restored.setStateInformation(resaved.getData(), int(resaved.getSize()));
    require(restored.loadRomFromFile(romFile), "restore global controller RAM");
    require(restored.getControllerSettings() == settings, "missing-ROM round trip restores controllers");
    require(restored.getMidiInputChannel() == 9, "missing-ROM resave keeps edited channel");
    require(p.getControllerSettings()[0] == 12, "instances have independent controllers");

    // Each MIDI controller must actually reach firmware modulation, not just change UI/RAM.
    bool allControllersAudible = true;
    for (int c = 0; c < 4; ++c)
    for (int assignment = 1; assignment <= 3; ++assignment)
    {
        std::array<std::vector<float>, 2> renders;
        for (int enabled = 0; enabled <= 1; ++enabled)
        {
            VDX7AudioProcessor synth(false);
            require(synth.loadRomFromFile(romFile), "controller audio ROM");
            synth.prepareToPlay(48000, 256);
            for (int source = 0; source < 4; ++source)
                for (int f = 1; f <= 3; ++f) synth.setControllerSettingFromUi(source, f, 0);
            for (int f = 1; f <= 3; ++f) synth.setControllerSettingFromUi(c, f, f == assignment ? 1 : 0);
            synth.setControllerSettingFromUi(c, 0, 0);
            set(synth, VDX7ParameterIDs::voiceParameter(VDX7VoiceData::VoiceParameter::pitchModSensitivity), 7);
            set(synth, VDX7ParameterIDs::voiceParameter(VDX7VoiceData::VoiceParameter::lfoSpeed), 50);
            for (int op = 0; op < 6; ++op)
                set(synth, VDX7ParameterIDs::operatorParameter(op,
                    VDX7VoiceData::Parameter::amplitudeModSensitivity), 3);
            const int input = assignment == 3 ? 64 : 127;
            for (int block = 0; block < 100; ++block)
            {
                // Change range after controller input and note-on. No fresh input
                // arrives after block 4: cached firmware modulation must refresh.
                if (block == 40)
                {
                    synth.setControllerSettingFromUi(c, 0, enabled ? 99 : 0);
                    // Equal refresh handshakes in both renders: differences must
                    // come from modulation, not from different CPU event schedules.
                    synth.setControllerSettingFromUi(c, assignment, 0);
                    synth.setControllerSettingFromUi(c, assignment, 1);
                }
                if (block == 4)
                {
                    midi.addEvent(juce::MidiMessage::noteOn(1, 60, uint8_t(100)), 0);
                    constexpr int cc[] { 1, 4, 2 };
                    midi.addEvent(c == 3 ? juce::MidiMessage::channelPressureChange(1, input)
                        : juce::MidiMessage::controllerEvent(1, cc[c], input), 0);
                }
                synth.processBlock(audio, midi);
                renders[enabled].insert(renders[enabled].end(), audio.getReadPointer(0),
                                        audio.getReadPointer(0) + audio.getNumSamples());
            }
            const auto runtime = ram(save(synth));
            require(static_cast<const uint8_t*>(runtime.getData())[0x1337 + 2 * c] == input * 2,
                    "settings refresh preserves raw controller input");
            const auto scaled = static_cast<const uint8_t*>(runtime.getData())[0x132f + 2 * c];
            require(enabled ? scaled > 0 : scaled == 0, "held controller recalculates scaled firmware input");
        }
        double difference = 0;
        for (size_t i = 0; i < renders[0].size(); ++i)
        {
            require(std::isfinite(renders[0][i]) && std::isfinite(renders[1][i]), "finite controller audio");
            const double delta = renders[0][i] - renders[1][i];
            difference += delta * delta;
        }
        std::cout << "Controller " << c << " assignment " << assignment
                  << " audio difference: " << difference << '\n';
        allControllersAudible &= difference > 0.00001;
    }
    require(allControllersAudible, "controller range changes rendered firmware audio");
}

int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    try
    {
        VDX7AudioProcessor original;
        {
            VDX7AudioProcessor noRom(false);
            std::unique_ptr<juce::AudioProcessorEditor> editor(noRom.createEditor());
            int saveButtons = 0;
            for (auto* child : editor->getChildren())
            {
                if (auto* button = dynamic_cast<juce::TextButton*>(child))
                    if (button->getButtonText() == "SAVE AS...")
                    {
                        ++saveButtons;
                        require(!button->isEnabled(), "Save As disabled without ROM");
                    }
                if (auto* box = dynamic_cast<juce::ComboBox*>(child))
                    if (box->getName() == "Algorithm")
                        require(!box->isEnabled(), "Algorithm disabled without ROM");
            }
            require(saveButtons == 1, "one persistent Save As button");
        }
        const auto explicitRom = juce::SystemStats::getEnvironmentVariable("VDX7_TEST_ROM_PATH", {});
        if (explicitRom.isNotEmpty())
            require(original.loadRomFromFile(juce::File(explicitRom)), "explicit local test ROM");
        require(original.getParameters().size() == 148, "148 host parameters");
        for (int op = 0; op < 6; ++op)
            for (int p = 0; p < 15; ++p)
            {
                const auto id = VDX7ParameterIDs::operatorParameter(
                    op, static_cast<VDX7VoiceData::Parameter>(p));
                require(original.parameters().getParameter(id)->getParameterIndex()
                            == 3 + op * 15 + p, "legacy parameter index changed");
            }
        if (!original.isRomLoaded())
        {
            std::cout << "SKIP: user ROM required for processor integration tests\n";
            return 77;
        }

        original.prepareToPlay(48000, 256);
        checkControllers(juce::File(original.getRomPath()));
        checkUserLibrary(juce::File(original.getRomPath()), argc > 1 ? juce::File(argv[1]) : juce::File());
        // Check patch/host coherence without creating an editor.
        original.selectProgramFromUi(3);
        auto state = save(original);
        auto bank = ram(state);
        require(original.getCurrentProgram() == 3, "pending program saved");
        const auto* voice = static_cast<const uint8_t*>(bank.getData()) + 3 * 128;
        for (int p = 0; p < VDX7VoiceData::kVoiceParameterCount; ++p)
        {
            const auto parameter = static_cast<VDX7VoiceData::VoiceParameter>(p);
            require(juce::roundToInt(value(original, VDX7ParameterIDs::voiceParameter(parameter)))
                        == VDX7VoiceData::getVoiceParameter(voice, 128, parameter),
                    "program sync without editor");
        }

        // Exercise all packed fields, then save before running another block.
        for (int op = 0; op < 6; ++op)
            for (int p = 0; p < VDX7VoiceData::kParameterCount; ++p)
            {
                const auto parameter = static_cast<VDX7VoiceData::Parameter>(p);
                const int lo = VDX7VoiceData::parameterMinimum(parameter);
                const int hi = VDX7VoiceData::parameterMaximum(parameter);
                set(original, VDX7ParameterIDs::operatorParameter(op, parameter),
                    float(lo + (op * 13 + p * 7) % (hi - lo + 1)));
            }
        for (int p = 0; p < VDX7VoiceData::kVoiceParameterCount; ++p)
        {
            const auto parameter = static_cast<VDX7VoiceData::VoiceParameter>(p);
            const int lo = VDX7VoiceData::voiceParameterMinimum(parameter);
            const int hi = VDX7VoiceData::voiceParameterMaximum(parameter);
            set(original, VDX7ParameterIDs::voiceParameter(parameter),
                float(lo + (p * 11) % (hi - lo + 1)));
        }
        state = save(original);
        VDX7AudioProcessor restored;
        restored.setStateInformation(state.getData(), int(state.getSize()));
        require(restored.getCurrentProgram() == 3, "program restore");
        require(ram(state) == ram(save(restored)), "RAM round trip before rendering");
        for (auto* parameter : original.getParameters())
        {
            auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(parameter);
            require(ranged != nullptr, "ranged parameter");
            require(std::abs(ranged->getValue()
                     - restored.parameters().getParameter(ranged->paramID)->getValue()) < 0.0001f,
                    "parameter state round trip");
        }

        // Reloading the UI must not discard a fresh edit while transport is stopped.
        require(restored.hasUnexportedEdits(), "dirty state survives project restore");
        require(restored.renameVoice("VDX TEST"), "rename voice");
        require(restored.getCurrentPatchName() == "VDX TEST", "name snapshot");
        require(!restored.renameVoice("TOO LONG NAME") && !restored.renameVoice(""), "invalid name rejected");
        auto beforeCopy = ram(save(restored));
        require(!restored.pasteOperator(1), "paste without clipboard rejected");
        require(restored.copyOperator(0) && restored.pasteOperator(5), "copy OP1 to OP6");
        auto afterCopy = ram(save(restored));
        const auto* a = static_cast<const uint8_t*>(beforeCopy.getData());
        const auto* b = static_cast<const uint8_t*>(afterCopy.getData());
        for (size_t i=0;i<afterCopy.getSize();++i)
            require(b[i] == (i>=3*128 && i<3*128+17 ? a[i+5*17] : a[i]),
                    "operator paste preserves other fields");
        juce::TemporaryFile singleFile(".syx"), bankFile(".syx"), badFile(".syx");
        juce::String error;
        require(restored.exportSyx(singleFile.getFile(),false,error), "single voice export");
        require(singleFile.getFile().getSize()==163 && !restored.hasUnexportedEdits(), "single export acknowledges voice");
        require(restored.renameVoice("EDIT AGAIN"), "second edit");
        restored.selectProgramFromUi(4);
        save(restored);
        require(!restored.isCurrentVoiceModified() && restored.hasUnexportedEdits(), "other voice remains dirty");
        auto beforeImport=ram(save(restored));
        require(restored.loadSyxFromFile(singleFile.getFile(),&error), "single voice import");
        auto afterImport=ram(save(restored));
        a=static_cast<const uint8_t*>(beforeImport.getData());
        b=static_cast<const uint8_t*>(afterImport.getData());
        for (size_t i=0;i<afterImport.getSize();++i)
            if (i<4*128 || i>=5*128) require(a[i]==b[i], "single import preserves other slots and system RAM");
        require(restored.getCurrentPatchName()=="VDX TEST", "single voice name imported");
        require(restored.hasUnexportedEdits(), "single import preserves other dirty flags");
        require(restored.exportSyx(bankFile.getFile(),true,error), "bank export");
        require(bankFile.getFile().getSize()==4104 && !restored.hasUnexportedEdits(), "bank export acknowledges all voices");
        auto exportedBank=ram(save(restored));
        require(restored.renameVoice("TEMP"), "edit before bank restore");
        require(restored.loadSyxFromFile(bankFile.getFile(),&error), "bank import");
        auto importedBank=ram(save(restored));
        require(std::memcmp(exportedBank.getData(),importedBank.getData(),4096)==0,"bank file round trip");
        require(badFile.getFile().replaceWithText("not sysex"), "invalid fixture write");
        require(!restored.loadSyxFromFile(badFile.getFile(),&error), "invalid import rejected");
        require(ram(save(restored))==importedBank,"invalid import preserves RAM");
        require(restored.renameVoice("UNSAVED"), "edit before failed export");
        require(!restored.exportSyx(badFile.getFile().getChildFile("missing/out.syx"),true,error)
                && restored.hasUnexportedEdits(), "failed export retains dirty flag");

        const auto speed = VDX7ParameterIDs::voiceParameter(VDX7VoiceData::VoiceParameter::lfoSpeed);
        set(restored, speed, 47);
        {
            std::unique_ptr<juce::AudioProcessorEditor> editor(restored.createEditor());
            require(editor->getWidth() == 1200 && editor->getHeight() == 925,
                    "Default editor includes extra chassis height");
            require(juce::roundToInt(value(restored, speed)) == 47, "editor erased pending automation");
            int switchIndex = 0;
            for (auto* child : editor->getChildren())
                if (auto* slider = dynamic_cast<juce::Slider*>(child))
                    if (bool(slider->getProperties().getWithDefault("vdx7SyncSwitch", false)))
                    {
                        require(switchIndex < 2, "Unexpected sync switch");
                        const auto id = VDX7ParameterIDs::voiceParameter(switchIndex++ == 0
                            ? VDX7VoiceData::VoiceParameter::oscillatorKeySync
                            : VDX7VoiceData::VoiceParameter::lfoKeySync);
                        const auto original = value(restored, id);
                        require(slider->getSliderStyle() == juce::Slider::LinearVertical,
                                "Sync switch must move vertically");
                        require(slider->getMinimum() == 0 && slider->getMaximum() == 1
                                && slider->getInterval() == 1, "Sync has exactly two positions");
                        for (int position : { 0, 1 })
                        {
                            set(restored, id, float(position));
                            require(slider->getValue() == position, "Automation updates sync switch");
                            slider->setValue(1 - position, juce::sendNotificationSync);
                            require(value(restored, id) == 1 - position, "Switch updates existing parameter");
                        }
                        set(restored, id, original);
                    }
            require(switchIndex == 2, "Two sync switches present");
            juce::Slider* modeSwitch = nullptr;
            for (auto* child : editor->getChildren())
                if (auto* slider = dynamic_cast<juce::Slider*>(child))
                    if (bool(slider->getProperties().getWithDefault("vdx7ModeSwitch", false)))
                        modeSwitch = slider;
            require(modeSwitch != nullptr, "Horizontal oscillator switch exists");
            require(modeSwitch->getSliderStyle() == juce::Slider::LinearHorizontal
                    && modeSwitch->getInterval() == 1, "Oscillator switch is binary/horizontal");
            const auto modeID = VDX7ParameterIDs::operatorParameter(
                0, VDX7VoiceData::Parameter::oscillatorMode);
            const auto originalMode = value(restored, modeID);
            for (int position : { 0, 1 })
            {
                set(restored, modeID, float(position));
                require(modeSwitch->getValue() == position, "Mode automation updates switch");
                modeSwitch->setValue(1 - position, juce::sendNotificationSync);
                require(value(restored, modeID) == 1 - position, "Switch updates oscillator mode");
            }
            set(restored, modeID, originalMode);
            for (const int width : { 960, 1440, 1600 })
            {
                editor->setSize(width, juce::roundToInt(width * 1110.0 / 1440.0));
                int lcdSelectors = 0;
                const float scale = float(width) / 1440.0f;
                const juce::Rectangle<float> lcdArea(382 * scale, 190 * scale,
                                                      550 * scale, 86 * scale);
                for (auto* child : editor->getChildren())
                    if (auto* box = dynamic_cast<juce::ComboBox*>(child))
                        if (bool(box->getProperties().getWithDefault("vdx7LcdCombo", false)))
                        {
                            ++lcdSelectors;
                            require(lcdArea.contains(box->getBounds().toFloat()),
                                    "LCD selector outside display");
                            require(box->findColour(juce::ComboBox::backgroundColourId).isTransparent(),
                                    "LCD selector obscures display background");
                        }
                require(lcdSelectors == 2, "Exactly two integrated LCD selectors");
                VDX7AlgorithmView* diagram=nullptr;
                for (auto* child:editor->getChildren())
                    if (auto* view=dynamic_cast<VDX7AlgorithmView*>(child)) diagram=view;
                require(diagram!=nullptr,"Algorithm view exists");
                juce::ComboBox* algorithmBox = nullptr;
                juce::TextButton* previous = nullptr;
                juce::TextButton* next = nullptr;
                for (auto* child : editor->getChildren())
                {
                    if (auto* box = dynamic_cast<juce::ComboBox*>(child))
                        if (box->getName() == "Algorithm") algorithmBox = box;
                    if (auto* button = dynamic_cast<juce::TextButton*>(child))
                    {
                        if (button->getButtonText() == "<") previous = button;
                        if (button->getButtonText() == ">") next = button;
                        if (button->getButtonText() == "SAVE AS...")
                            require(button->isEnabled(), "Save As enabled with ROM");
                    }
                    if (auto* label = dynamic_cast<juce::Label*>(child))
                        require(label->getText() != "ALGO", "duplicate algorithm encoder removed");
                }
                require(algorithmBox && algorithmBox->getNumItems() == 32,
                        "numbered 32-algorithm selector");
                require(previous && next, "LCD navigation buttons present");
                require(previous->getY() > int(180 * scale) && next->getY() > int(180 * scale),
                        "preset arrows moved out of header into LCD row");
                require(!previous->getBounds().intersects(next->getBounds()), "separate LCD arrows");
                for (auto* arrow : { previous, next })
                {
                    require(bool(arrow->getProperties().getWithDefault("vdx7LcdArrow", false)),
                            "LCD arrow uses integrated display style");
                    require(lcdArea.contains(arrow->getBounds().toFloat()), "LCD arrow contained in display");
                    for (auto* child : editor->getChildren())
                        if (auto* box = dynamic_cast<juce::ComboBox*>(child))
                            if (bool(box->getProperties().getWithDefault("vdx7LcdCombo", false)))
                                require(!arrow->getBounds().intersects(box->getBounds()),
                                        "LCD arrow does not overlap selector hit area");
                }
                restored.selectProgramFromUi(0);
                save(restored);
                previous->onClick();
                save(restored);
                require(restored.getCurrentProgram() == 31, "LCD previous wraps 01 to 32");
                next->onClick();
                save(restored);
                require(restored.getCurrentProgram() == 0, "LCD next wraps 32 to 01");
                const auto algoID=VDX7ParameterIDs::voiceParameter(VDX7VoiceData::VoiceParameter::algorithm);
                for (int algorithm=1;algorithm<=32;++algorithm)
                {
                    set(restored,algoID,float(algorithm));
                    require(diagram->algorithm()==algorithm,"Algorithm parameter updates diagram");
                    require(algorithmBox->getSelectedId() == algorithm, "automation updates dropdown");
                    auto beforeSelection=ram(save(restored));
                    for (int op=0;op<6;++op)
                    {
                        const auto bounds=diagram->nodeBounds(op);
                        require(diagram->getLocalBounds().toFloat().contains(bounds),"Node fits diagram");
                        require(diagram->operatorAt(bounds.getCentre())==op,"Node hit test");
                        diagram->onOperatorSelected(op);
                        require(modeSwitch->getValue() == value(restored,
                            VDX7ParameterIDs::operatorParameter(op,
                                VDX7VoiceData::Parameter::oscillatorMode)),
                            "Mode switch follows selected operator");
                        require(diagram->selectedOperator()==op,"Diagram selection updates editor");
                        for (int other=op+1;other<6;++other)
                            require(!bounds.intersects(diagram->nodeBounds(other)),"Nodes do not overlap");
                    }
                    require(beforeSelection==ram(save(restored)),"Selecting node does not edit voice RAM");
                }
                for (int algorithm = 32; algorithm >= 1; --algorithm)
                {
                    algorithmBox->setSelectedId(algorithm, juce::sendNotificationSync);
                    require(juce::roundToInt(value(restored, algoID)) == algorithm,
                            "dropdown updates existing host parameter");
                    require(diagram->algorithm() == algorithm, "dropdown updates diagram immediately");
                    const auto packed = ram(save(restored));
                    const auto* selectedVoice = static_cast<const uint8_t*>(packed.getData())
                        + restored.getCurrentProgram() * 128;
                    require(VDX7VoiceData::getVoiceParameter(selectedVoice, 128,
                        VDX7VoiceData::VoiceParameter::algorithm) == algorithm,
                        "dropdown commits to firmware voice RAM");
                }
                set(restored,algoID,4);
                juce::TextButton* performanceTab = nullptr;
                juce::TextButton* editTab = nullptr;
                VDX7PerformancePanel* performance = nullptr;
                for (auto* child : editor->getChildren())
                {
                    if (auto* panel = dynamic_cast<VDX7PerformancePanel*>(child)) performance = panel;
                    if (auto* button = dynamic_cast<juce::TextButton*>(child))
                    {
                        if (button->getButtonText() == "PERFORMANCE") performanceTab = button;
                        if (button->getButtonText() == "EDIT") editTab = button;
                    }
                }
                require(performanceTab && editTab && performance, "performance view exists");
                // Earlier exhaustive algorithm sweeps deliberately queued many
                // program reloads without audio. Let the host consume them.
                {
                    juce::AudioBuffer<float> audio(2, 256);
                    juce::MidiBuffer midi;
                    for (int block = 0; block < 4000; ++block) restored.processBlock(audio, midi);
                }
                require(!performance->isVisible(), "editor starts in edit view");
                performanceTab->onClick();
                require(performance->isVisible() && !modeSwitch->isVisible(), "performance replaces operator controls");
                require(previous->isVisible() && next->isVisible() && algorithmBox->isVisible(),
                        "performance keeps LCD navigation and algorithm");
                int ranges = 0, assignments = 0, bendFields = 0, playFields = 0;
                const auto voiceBeforePerformance = ram(save(restored));
                for (auto* child : performance->getChildren())
                {
                    require(performance->getLocalBounds().contains(child->getBounds())
                            && !child->getBounds().isEmpty(), "performance control bounds");
                    if (auto* box = dynamic_cast<juce::ComboBox*>(child))
                    {
                        if (!box->getName().startsWith("Pitch bend"))
                        {
                            const int field = box->getName() == "Play mode" ? 0
                                : box->getName() == "Portamento mode" ? 1
                                : box->getName() == "Glissando" ? 2 : 3;
                            const float groupWidth = performance->getWidth() * 0.83f / 3;
                            require(field == 0 ? box->getRight() < groupWidth
                                               : box->getX() > 2 * groupWidth,
                                    "play and portamento controls stay in their visual groups");
                            box->setSelectedId(2, juce::sendNotificationSync);
                            if (restored.getPlaySettings()[field] != 1)
                                std::cerr << "Play GUI field " << field << " name " << box->getName()
                                          << " selected " << box->getSelectedId() << " actual " << restored.getPlaySettings()[field] << '\n';
                            require(restored.getPlaySettings()[field] == 1, "play GUI binding");
                            ++playFields;
                            continue;
                        }
                        const int field = box->getName() == "Pitch bend range" ? 0 : 1;
                        require(box->getNumItems() == 13, "bend choices 0-12");
                        box->setSelectedId(6, juce::sendNotificationSync);
                        require(restored.getPitchBendSettings()[field] == 5, "bend GUI writes firmware");
                        ++bendFields;
                    }
                    if (auto* slider = dynamic_cast<juce::Slider*>(child))
                    {
                        slider->setValue(25 + ranges, juce::sendNotificationSync);
                        require(restored.getControllerSettings()[ranges * 4] == 25 + ranges,
                                "performance range knob writes firmware setting");
                        ++ranges;
                    }
                    if (auto* button = dynamic_cast<juce::ToggleButton*>(child))
                    {
                        const int c = assignments / 3, f = assignments % 3 + 1;
                        for (bool enabled : { false, true })
                        {
                            button->setToggleState(enabled, juce::dontSendNotification);
                            button->onClick();
                            require(restored.getControllerSettings()[c * 4 + f] == int(enabled),
                                    "performance assignment writes firmware setting");
                        }
                        ++assignments;
                    }
                }
                require(ranges == 4 && assignments == 12, "complete four-controller matrix");
                require(bendFields == 2, "two pitch-bend selectors");
                require(playFields == 4, "four play/portamento selectors");
                const auto afterPerformance = ram(save(restored));
                require(std::memcmp(voiceBeforePerformance.getData(), afterPerformance.getData(), 4096) == 0,
                        "performance UI preserves voice bank");
                restored.setControllerSettingFromUi(0, 0, 61);
                performance->refresh();
                for (auto* child : performance->getChildren())
                    if (auto* slider = dynamic_cast<juce::Slider*>(child))
                        if (slider->getName() == "Controller 0 range")
                            require(slider->getValue() == 61, "performance refresh reads firmware state");
                if (argc > 1)
                {
                    const auto path = juce::File(argv[1]).getChildFile("VDX7-performance-" + juce::String(width) + ".png");
                    auto stream = path.createOutputStream();
                    require(stream && stream->setPosition(0) && stream->truncate().wasOk(), "performance snapshot file");
                    require(juce::PNGImageFormat().writeImageToStream(
                        editor->createComponentSnapshot(editor->getLocalBounds()), *stream), "performance render");
                }
                editTab->onClick();
                require(!performance->isVisible() && modeSwitch->isVisible(), "return to edit view");
                for (auto* child : editor->getChildren())
                    if (child->isVisible())
                        require(!child->getBounds().isEmpty()
                                && editor->getLocalBounds().contains(child->getBounds()),
                                "visible control outside editor or missing bounds");
                if (argc > 1)
                {
                    auto path = juce::File(argv[1]).getChildFile(
                        "VDX7-v0.6.6-" + juce::String(width) + ".png");
                    auto stream = path.createOutputStream();
                    require(stream != nullptr, "snapshot file");
                    require(stream->setPosition(0) && stream->truncate().wasOk(), "snapshot reset");
                    juce::PNGImageFormat png;
                    require(png.writeImageToStream(
                        editor->createComponentSnapshot(editor->getLocalBounds()), *stream),
                        "snapshot render");
                }
            }
        }

        if (argc>1)
        {
            juce::Image sheet(juce::Image::RGB,960,1312,true);
            juce::Graphics g(sheet);
            g.fillAll(juce::Colour(0xff0b191f));
            for (int a=1;a<=32;++a)
            {
                const int x=((a-1)%4)*240, y=((a-1)/4)*164;
                VDX7AlgorithmView view;
                view.setSize(200,124);
                view.setState(a,(a-1)%6,7);
                g.setColour(juce::Colour(0xffe0eef0));
                g.setFont(juce::Font(juce::FontOptions(14.0f)));
                g.drawText("ALGORITHM "+juce::String(a),x+12,y+4,210,24,juce::Justification::centredLeft);
                g.drawImageAt(view.createComponentSnapshot(view.getLocalBounds()),x+12,y+28);
            }
            auto file=juce::File(argv[1]).getChildFile("VDX7-v0.6.6-algorithms.png");
            auto stream=file.createOutputStream();
            require(stream && stream->setPosition(0) && stream->truncate().wasOk(),"Algorithm sheet file");
            require(juce::PNGImageFormat().writeImageToStream(sheet,*stream),"Algorithm sheet render");
        }

        {
            auto renderWheel=[](float value)
            {
                juce::Image image(juce::Image::ARGB,48,138,true);
                juce::Graphics graphics(image);
                VDX7MechanicalDrawing::wheel(graphics,{0,0,48,138},value,false);
                return image;
            };
            auto a=renderWheel(0.30f), b=renderWheel(0.31f), repeated=renderWheel(0.30f);
            int movingRibPixels=0;
            for (int y=0;y<138;++y) for (int x=0;x<48;++x)
            {
                require(a.getPixelAt(x,y)==repeated.getPixelAt(x,y),"Wheel rendering is deterministic");
                if (x<5 || x>42) require(a.getPixelAt(x,y)==b.getPixelAt(x,y),"Wheel frame is stationary");
                // This region excludes the cyan stripe at these two positions.
                if (x>12 && x<35 && y>25 && y<60 && a.getPixelAt(x,y)!=b.getPixelAt(x,y))
                    ++movingRibPixels;
            }
            require(movingRibPixels>20,"Actual rib texture moves, not only the marker");
        }
        if (argc>1)
        {
            juce::Image sheet(juce::Image::RGB,800,320,true);
            juce::Graphics g(sheet); g.fillAll(juce::Colour(0xff102026));
            for (int frame=0;frame<9;++frame)
            {
                const float x=20.0f+frame*85.0f;
                VDX7MechanicalDrawing::wheel(g,{x,35,64,220},frame/8.0f,false);
                g.setColour(juce::Colours::white);
                g.setFont(juce::Font(juce::FontOptions(12.0f)));
                g.drawText(juce::String(frame*12.5f,1)+"%",int(x),10,64,20,juce::Justification::centred);
            }
            VDX7MechanicalDrawing::faderCap(g,{45,275,90,38},false,false);
            VDX7MechanicalDrawing::faderCap(g,{175,275,90,38},true,false);
            VDX7MechanicalDrawing::faderCap(g,{305,275,90,38},true,true);
            auto file=juce::File(argv[1]).getChildFile("VDX7-v0.6.6-wheel-frames.png");
            auto stream=file.createOutputStream();
            require(stream && stream->setPosition(0) && stream->truncate().wasOk(),"Wheel sheet file");
            require(juce::PNGImageFormat().writeImageToStream(sheet,*stream),"Wheel sheet render");
        }
        // A legacy/RAM-only state must restore fields absent from its parameter tree.
        auto legacyXml = juce::AudioProcessor::getXmlFromBinary(state.getData(), int(state.getSize()));
        auto legacy = juce::ValueTree::fromXml(*legacyXml);
        legacy.removeAllChildren(nullptr);
        juce::MemoryBlock legacyState;
        juce::AudioProcessor::copyXmlToBinary(*legacy.createXml(), legacyState);
        restored.setStateInformation(legacyState.getData(), int(legacyState.getSize()));
        require(ram(state) == ram(save(restored)), "RAM-only state restoration");

        // Run a factory voice through real firmware without opening an audio device.
        VDX7AudioProcessor render;
        for (const double rate : { 44100.0, 48000.0, 96000.0 })
        for (const int size : { 64, 128, 256 })
        {
            render.prepareToPlay(rate, size);
            juce::AudioBuffer<float> audio(2, size);
            juce::MidiBuffer midi;
            float peak = 0.0f;
            for (int block = 0; block < int(rate) / size; ++block)
            {
                if (block == 20)
                    midi.addEvent(juce::MidiMessage::noteOn(1, 60, uint8_t(100)), 0);
                render.processBlock(audio, midi);
                peak = std::max(peak, audio.getMagnitude(0, 0, size));
                for (int ch = 0; ch < 2; ++ch)
                    for (int sample = 0; sample < size; ++sample)
                        require(std::isfinite(audio.getSample(ch, sample)), "non-finite audio");
            }
            require(peak > 0.00001f, "silent factory voice render");
            midi.addEvent(juce::MidiMessage::allNotesOff(1), 0);
            render.processBlock(audio, midi);
        }
        std::cout << "PASS: legacy indices, headless program sync, 145 voice values, RAM/state round trip, "
                     "32 diagrams and all node hit tests at three sizes, nonmutating operator selection, "
                     "rename, copy/paste isolation, single/bank file export/import, failed I/O protection, dirty tracking, "
                     "RAM-only restore, stopped-transport edit, editor bounds, non-silent finite "
                     "44.1/48/96 kHz rendering at 64/128/256 samples\n";
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
