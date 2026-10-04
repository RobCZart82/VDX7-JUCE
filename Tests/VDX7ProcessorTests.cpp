#include "PluginEditor.h"
#include "VDX7AboutPanel.h"
#include "VDX7Sysex.h"
#include "VDX7MechanicalDrawing.h"
#include <iostream>
#include <memory>
#include <cmath>
#include <stdexcept>

// Processor fixtures are heap-owned: several live instances (including nested
// helpers) exceed the default Windows stack. References keep assertions intact.
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

static std::pair<int, int> restoredPreRomVoiceValues(VDX7AudioProcessor& processor)
{
    const auto state = save(processor);
    auto xml = juce::AudioProcessor::getXmlFromBinary(state.getData(), int(state.getSize()));
    require(xml != nullptr, "decode restored no-ROM state");
    const auto tree = juce::ValueTree::fromXml(*xml);
    juce::MemoryBlock packedRam;
    require(packedRam.fromBase64Encoding(tree.getProperty("ram").toString()),
            "decode restored no-ROM RAM");
    const auto* bytes = static_cast<const uint8_t*>(packedRam.getData());
    return {
        VDX7VoiceData::getVoiceParameter(bytes, packedRam.getSize(),
            VDX7VoiceData::VoiceParameter::feedback),
        VDX7VoiceData::getOperatorParameter(bytes, packedRam.getSize(), 5,
            VDX7VoiceData::Parameter::outputLevel)
    };
}

static void testPreRomParameterEditsSurviveSave(const juce::File& romFile)
{
    const auto feedbackId = VDX7ParameterIDs::voiceParameter(
        VDX7VoiceData::VoiceParameter::feedback);
    const auto outputId = VDX7ParameterIDs::operatorParameter(5,
        VDX7VoiceData::Parameter::outputLevel);

    // Saving must not consume the pending edit before the first ROM can accept it.
    auto sameInstance = std::make_unique<VDX7AudioProcessor>(false);
    set(*sameInstance, feedbackId, 6.0f);
    set(*sameInstance, outputId, 42.0f);
    auto saved = save(*sameInstance);
    require(sameInstance->loadRomFromFile(romFile), "first ROM after no-ROM save");
    const auto sameInstanceValues = restoredPreRomVoiceValues(*sameInstance);
    require(sameInstanceValues.first == 6 && sameInstanceValues.second == 42,
            "same-instance first ROM load applies pre-save host edits");

    // Restoring a no-ROM project suppresses parameter callbacks by design;
    // explicit values therefore have to travel in DeferredVoiceEdits.
    auto reopened = std::make_unique<VDX7AudioProcessor>(false);
    reopened->setStateInformation(saved.getData(), int(saved.getSize()));
    require(reopened->loadRomFromFile(romFile), "first ROM after reopening no-ROM project");
    const auto reopenedValues = restoredPreRomVoiceValues(*reopened);
    require(reopenedValues.first == 6 && reopenedValues.second == 42,
            "reopened no-ROM project applies saved host edits on first ROM load");
    std::cout << "PASS: explicit pre-ROM host edits survive save and first ROM load\n";
}

static void testPreRomDeferredStateIsSerialized()
{
    const auto feedbackId = VDX7ParameterIDs::voiceParameter(
        VDX7VoiceData::VoiceParameter::feedback);
    const auto outputId = VDX7ParameterIDs::operatorParameter(5,
        VDX7VoiceData::Parameter::outputLevel);
    auto source = std::make_unique<VDX7AudioProcessor>(false);
    set(*source, feedbackId, 6.0f);
    set(*source, outputId, 42.0f);
    const auto saved = save(*source);

    auto verifyDeferredValues = [&](const juce::MemoryBlock& bytes)
    {
        auto xml = juce::AudioProcessor::getXmlFromBinary(bytes.getData(), int(bytes.getSize()));
        require(xml != nullptr, "decode no-ROM deferred project");
        const auto state = juce::ValueTree::fromXml(*xml);
        require(state.getProperty("ram").toString().isEmpty(), "no-ROM project has no packed RAM");
        const auto edits = state.getChildWithName("DeferredVoiceEdits");
        require(edits.isValid()
                && static_cast<int>(edits.getProperty("voice9", -1)) == 6
                && static_cast<int>(edits.getProperty("op113", -1)) == 42,
                "no-ROM project serializes explicit feedback and OP6 level edits");
    };
    verifyDeferredValues(saved);

    auto reopened = std::make_unique<VDX7AudioProcessor>(false);
    reopened->setStateInformation(saved.getData(), int(saved.getSize()));
    verifyDeferredValues(save(*reopened));
    std::cout << "PASS: no-ROM explicit voice edits survive save and state restore\n";
}

// Project restore preserves persistent data, not CPU stack/working tables or
// live voice ownership. In-place nonmutation tests below still compare all RAM.
static bool sameProjectRam(const juce::MemoryBlock& a, const juce::MemoryBlock& b)
{
    if (a.getSize() != 6144 || b.getSize() != 6144) return false;
    const auto* x = static_cast<const uint8_t*>(a.getData());
    const auto* y = static_cast<const uint8_t*>(b.getData());
    if (std::memcmp(x, y, 4096) != 0) return false;
    for (const int address : {0x20a9, 0x20aa, 0x20ab, 0x257d, 0x2311, 0x2312,
                              0x2328, 0x2329, 0x232e, 0x2330, 0x2332, 0x2334,
                              0x2336, 0x2338, 0x233a, 0x233c})
        if (x[address - 0x1000] != y[address - 0x1000]) return false;
    return true;
}

static void checkProjectRamOracle(const juce::MemoryBlock& source)
{
    require(sameProjectRam(source, source), "project RAM oracle positive control");
    auto mutant = source;
    auto* bytes = static_cast<uint8_t*>(mutant.getData());
    for (int i = 0; i < 4096; ++i)
    {
        bytes[i] ^= 1;
        require(!sameProjectRam(source, mutant), "project RAM oracle missed changed voice byte");
        bytes[i] ^= 1;
    }
    for (const int address : {0x20a9, 0x20aa, 0x20ab, 0x257d, 0x2311, 0x2312,
                              0x2328, 0x2329, 0x232e, 0x2330, 0x2332, 0x2334,
                              0x2336, 0x2338, 0x233a, 0x233c})
    {
        bytes[address - 0x1000] ^= 1;
        require(!sameProjectRam(source, mutant), "project RAM oracle missed setting loss");
        bytes[address - 0x1000] ^= 1;
    }
}

static void checkUserLibrary(const juce::File& romFile, const juce::File& imageFolder)
{
    juce::TemporaryFile temporary;
    const auto file = temporary.getFile();
    auto pStorage = std::make_unique<VDX7AudioProcessor>(false);
    auto& p = *pStorage;
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
        require(tuning->getComboBoxComponent("monoCorrection")
                && tuning->getComboBoxComponent("monoCorrection")->getNumItems() == 2
                && tuning->getComboBoxComponent("monoCorrection")->getSelectedItemIndex() == 0,
                "SETTINGS exposes native-default MONO correction choice");
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
        auto* credits = dynamic_cast<juce::DialogWindow*>(juce::Component::getCurrentlyModalComponent());
        require(credits != nullptr, "ABOUT opens");
        auto* panel = dynamic_cast<VDX7AboutPanel*>(credits->getContentComponent());
        require(panel && panel->hasVectorLogos(), "ABOUT embeds both vector logos");
        juce::TextButton* closeAbout = nullptr;
        bool sourceLink = false;
        for (auto* child : panel->getChildren())
        {
            require(panel->getLocalBounds().contains(child->getBounds()), "ABOUT content bounds");
            if (auto* link = dynamic_cast<juce::HyperlinkButton*>(child))
                sourceLink = link->getURL().toString(false) == "https://github.com/RobCZart82/VDX7-JUCE";
            if (auto* button = dynamic_cast<juce::TextButton*>(child)) closeAbout = button;
        }
        require(sourceLink && closeAbout, "ABOUT source link and close action present");
        if (imageFolder != juce::File())
        {
            juce::FileOutputStream stream(imageFolder.getChildFile("VDX7-about.png"));
            require(stream.openedOk() && juce::PNGImageFormat().writeImageToStream(
                credits->createComponentSnapshot(credits->getLocalBounds()), stream), "about screenshot");
            juce::FileOutputStream retina(imageFolder.getChildFile("VDX7-about-2x.png"));
            require(retina.openedOk() && juce::PNGImageFormat().writeImageToStream(
                panel->createComponentSnapshot(panel->getLocalBounds(), true, 2.0f), retina),
                "ABOUT double-resolution vector render");
        }
        editor.reset(); // About owns its assets/LAF and must not depend on the editor.
        require(panel->hasVectorLogos(), "ABOUT survives editor destruction");
        closeAbout->onClick();
        require(!credits->isCurrentlyModal(), "ABOUT OK closes modal state");
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
    auto restoredStorage = std::make_unique<VDX7AudioProcessor>(false);
    auto& restored = *restoredStorage;
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
    auto pStorage = std::make_unique<VDX7AudioProcessor>(false);
    auto& p = *pStorage;
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
        auto restoredStorage = std::make_unique<VDX7AudioProcessor>(false);
        auto& restored = *restoredStorage;
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
        auto restoredStorage = std::make_unique<VDX7AudioProcessor>(false);
        auto& restored = *restoredStorage;
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
        auto otherStorage = std::make_unique<VDX7AudioProcessor>(false);
        auto& other = *otherStorage;
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
    auto missingStorage = std::make_unique<VDX7AudioProcessor>(false);
    auto& missing = *missingStorage;
    missing.setStateInformation(missingData.getData(), int(missingData.getSize()));
    require(!missing.isRomLoaded(), "deliberate missing ROM");
    require(missing.getMidiInputChannel() == 12, "missing-ROM channel recall");
    require(missing.setMidiInputChannelFromUi(9), "edit channel while ROM missing");
    require(!missing.setControllerSettingFromUi(0, 0, 1), "missing ROM preserves pending globals");
    const auto resaved = save(missing);
    auto restoredStorage = std::make_unique<VDX7AudioProcessor>(false);
    auto& restored = *restoredStorage;
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
            auto synthStorage = std::make_unique<VDX7AudioProcessor>(false);
            auto& synth = *synthStorage;
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

static std::vector<uint8_t> syntheticLegacyBank()
{
    std::vector<uint8_t> bank(4096,0);
    for (int slot = 0; slot < 32; ++slot)
        std::fill_n(bank.begin() + slot * 128 + 118,10,uint8_t('L'));
    bank[74] = 127;                  // OP2 EG L3 in the first voice.
    bank[128] = 127;                 // OP6 EG R1 in the second voice.
    bank[128 + 101] = 100;           // OP1 fine in the second voice.
    bank[31 * 128 + 24] = 127;       // OP5 EG L4 in the last voice.
    return bank;
}

static void testLegacyPendingProjectState()
{
    const auto bank = syntheticLegacyBank();
    auto source = std::make_unique<VDX7AudioProcessor>(false);
    const auto initial = save(*source);
    auto xml = juce::AudioProcessor::getXmlFromBinary(initial.getData(),int(initial.getSize()));
    require(xml != nullptr,"legacy pending state XML");
    auto tree = juce::ValueTree::fromXml(*xml);
    juce::MemoryBlock memory(VDX7Engine::kRamStateSize,true);
    std::memcpy(memory.getData(),bank.data(),bank.size());
    tree.setProperty("ram",memory.toBase64Encoding(),nullptr);
    tree.setProperty("romPath",juce::String(),nullptr);
    tree.setProperty("romIdentity",juce::String(),nullptr);
    juce::MemoryBlock state;
    juce::AudioProcessor::copyXmlToBinary(*tree.createXml(),state);
    auto reopened = std::make_unique<VDX7AudioProcessor>(false);
    reopened->setStateInformation(state.getData(),int(state.getSize()));
    require(ram(save(*reopened)) == memory,"legacy raw values survive pending project restore without ROM");
    std::cout << "PASS: legacy pending project state retains raw EG/fine bytes (no ROM)\n";
}

static void testLegacyProcessorImport(const juce::File& romFile)
{
    const auto bank = syntheticLegacyBank();
    const auto message = VDX7Sysex::encode(bank);
    require(message.size() == 4104,"legacy synthetic VMEM encoder");
    juce::TemporaryFile file(".syx");
    require(file.getFile().replaceWithData(message.data(),message.size()),"legacy import fixture");
    auto source = std::make_unique<VDX7AudioProcessor>(false);
    require(source->loadRomFromFile(romFile),"legacy processor test ROM");
    juce::String error;
    require(source->loadSyxFromFile(file.getFile(),&error),"legacy complete bank import");
    auto checkPacked = [&](const juce::MemoryBlock& state) {
        const auto memory = ram(state);
        require(memory.getSize() == VDX7Engine::kRamStateSize
                && std::memcmp(memory.getData(),bank.data(),bank.size()) == 0,
                "processor import/publication/project save preserve raw bank bytes");
    };
    for (int slot : {0,1,31})
    {
        source->selectProgramFromUi(slot);
        source->synchroniseOperatorParametersFromEngine();
        checkPacked(save(*source));
        VDX7AudioProcessor::SyxExportSnapshot single;
        require(source->captureSyxExportSnapshot(false,single,error),"legacy single export capture");
        std::vector<uint8_t> decoded;
        require(VDX7Sysex::decode(single.message(),decoded)
                && std::equal(decoded.begin(),decoded.end(),bank.begin() + slot * 128),
                "processor single export retains raw legacy values");
    }
    VDX7AudioProcessor::SyxExportSnapshot full;
    require(source->captureSyxExportSnapshot(true,full,error) && full.message() == message,
            "processor bank export preserves the complete input message");
    const auto state = save(*source);
    auto restored = std::make_unique<VDX7AudioProcessor>(false);
    restored->setStateInformation(state.getData(),int(state.getSize()));
    require(restored->isRomLoaded(),"legacy restored processor has firmware");
    checkPacked(save(*restored));

    // A checksum-valid invalid value remains a transactional rejection.
    auto bad = message;
    bad[6 + 14] = 100; // Output level: no legacy exception.
    unsigned checksum = 0;
    for (std::size_t i = 6; i < bad.size()-2; ++i) checksum += bad[i];
    bad[bad.size()-2] = static_cast<uint8_t>((128-(checksum&127))&127);
    require(file.getFile().replaceWithData(bad.data(),bad.size()),"legacy negative fixture");
    const auto before = ram(save(*restored));
    require(!restored->loadSyxFromFile(file.getFile(),&error),"reject invalid checksum-valid bank after legacy import");
    require(ram(save(*restored)) == before,"failed import preserves legacy RAM transactionally");
    std::cout << "PASS: legacy import, passive sync, single/bank export, project restore and negative import\n";
}

static void testPendingFactoryCatalog()
{
    auto processor = std::make_unique<VDX7AudioProcessor>(false);
    auto empty = save(*processor);
    auto xml = juce::AudioProcessor::getXmlFromBinary(empty.getData(), int(empty.getSize()));
    auto tree = juce::ValueTree::fromXml(*xml);
    require(!tree.hasProperty("factoryBankMask"), "first no-ROM save does not freeze an unknown catalog");
    VDX7FactoryBanks::Snapshot banks;
    banks.image.resize(32768, 0);
    banks.mask = 4;
    banks.image[2*4096+14] = 75;
    VDX7FactoryBanks::writeState(tree, banks);
    juce::MemoryBlock state;
    juce::AudioProcessor::copyXmlToBinary(*tree.createXml(), state);
    processor->setStateInformation(state.getData(), int(state.getSize()));
    const auto preserved = save(*processor);
    auto savedXml = juce::AudioProcessor::getXmlFromBinary(preserved.getData(), int(preserved.getSize()));
    VDX7FactoryBanks::Snapshot restored;
    bool present = false;
    require(VDX7FactoryBanks::readState(juce::ValueTree::fromXml(*savedXml), restored, present)
        && present && restored.image == banks.image && restored.mask == banks.mask,
        "missing firmware retains complete project catalog");
    for (int mutation = 0; mutation < 5; ++mutation)
    {
        auto bad = tree.createCopy();
        if (mutation == 0) bad.removeProperty("factoryBankMask", nullptr);
        if (mutation == 1) bad.setProperty("factoryBankMask", 256, nullptr);
        if (mutation == 2) bad.setProperty("factoryBanks", "broken", nullptr);
        if (mutation == 3) bad.setProperty("factoryBanks", "-1.", nullptr);
        if (mutation == 4) bad.setProperty("ram", "2147483647.", nullptr);
        juce::AudioProcessor::copyXmlToBinary(*bad.createXml(), state);
        processor->setStateInformation(state.getData(), int(state.getSize()));
        require(save(*processor) == preserved, "invalid catalog cannot replace pending project");
    }
}

static void testFactoryBankFolder(const juce::File& rom, const juce::File& sourceFolder)
{
    // Private local data only; nothing from these user files enters the repository.
    const auto scan = VDX7FactoryBanks::scan(sourceFolder);
    require(scan.banks.mask == 255, "local eight reference banks recognised");
    juce::MemoryBlock firmware;
    require(rom.loadFileAsData(firmware) && firmware.getSize() >= 16384, "private firmware fixture");
    juce::TemporaryFile firmwareFile(".rom");
    require(firmwareFile.getFile().replaceWithData(firmware.getData(), 16384), "firmware-only local fixture");
    const auto folder = juce::File::getSpecialLocation(juce::File::tempDirectory)
        .getNonexistentChildFile("vdx7-private-library", {}, false);
    struct Cleanup { juce::File folder; ~Cleanup() { folder.deleteRecursively(); } } cleanup {folder};
    require(folder.createDirectory().wasOk(), "private isolated bank folder");
    auto first = VDX7Sysex::encode(std::vector<uint8_t>(scan.banks.image.begin()+2*4096, scan.banks.image.begin()+3*4096));
    const auto renamed = folder.getChildFile("not-rom2a.SYX");
    require(renamed.replaceWithData(first.data(), first.size()), "renamed reference fixture");
    auto processor = std::make_unique<VDX7AudioProcessor>(false);
    require(processor->loadRomFromFile(firmwareFile.getFile()), "firmware without combined banks");
    require(!processor->hasFactoryVoices(), "firmware-only starts with disabled slots");
    juce::String report;
    const auto initialRam = ram(save(*processor));
    require(processor->refreshFactoryBanks(folder, report), "scan partial local folder");
    require(!processor->refreshFactoryBanks(renamed, report) && processor->hasFactoryBank(2), "invalid folder leaves catalog intact");
    auto automatic = std::make_unique<VDX7AudioProcessor>(false, folder);
    require(automatic->loadRomFromFile(firmwareFile.getFile()) && automatic->hasFactoryBank(2)
        && !automatic->hasFactoryBank(0) && automatic->getCurrentBank() == 2,
        "first firmware-only load automatically scans folder and selects first available slot");
    require(ram(save(*processor)) == initialRam, "refresh preserves working RAM");
    for (int i = 0; i < 8; ++i) require(processor->hasFactoryBank(i) == (i == 2), "only recognised slot enabled");
    require(!processor->selectFactoryBank(0) && processor->selectFactoryBank(2), "missing bank rejected, recognised bank selectable");
    processor->selectProgramFromUi(7);
    save(*processor); // commit the bank selection before changing every field
    for (int op = 0; op < 6; ++op)
        for (int p = 0; p < VDX7VoiceData::kParameterCount; ++p)
        {
            const auto parameter = static_cast<VDX7VoiceData::Parameter>(p);
            const int lo = VDX7VoiceData::parameterMinimum(parameter), hi = VDX7VoiceData::parameterMaximum(parameter);
            set(*processor, VDX7ParameterIDs::operatorParameter(op, parameter), float(lo + (op*17+p*3)%(hi-lo+1)));
        }
    for (int p = 0; p < VDX7VoiceData::kVoiceParameterCount; ++p)
    {
        const auto parameter = static_cast<VDX7VoiceData::VoiceParameter>(p);
        const int lo = VDX7VoiceData::voiceParameterMinimum(parameter), hi = VDX7VoiceData::voiceParameterMaximum(parameter);
        set(*processor, VDX7ParameterIDs::voiceParameter(parameter), float(lo+(p*7)%(hi-lo+1)));
    }
    require(processor->renameVoice("LAST EDIT"), "latest patch name");
    const auto state = save(*processor); // no intervening processBlock
    const auto editedRam = ram(state);
    require(renamed.deleteFile(), "remove isolated library file");
    require(processor->refreshFactoryBanks(folder, report) && !processor->hasFactoryVoices(), "removed library disables slots");
    require(ram(save(*processor)) == editedRam && processor->hasUnexportedEdits(), "refresh cannot erase latest patch or dirty flag");
    const auto changedFolderBank = VDX7Sysex::encode(std::vector<uint8_t>(scan.banks.image.begin(), scan.banks.image.begin()+4096));
    require(folder.getChildFile("rom2a.syx").replaceWithData(changedFolderBank.data(), changedFolderBank.size()), "different local folder generation");
    std::vector<uint8_t> combined(49152, 0); // Synthetic legacy bank data with the private firmware.
    std::memcpy(combined.data(), firmware.getData(), 16384);
    juce::TemporaryFile combinedFile(".rom");
    require(combinedFile.getFile().replaceWithData(combined.data(), combined.size()), "isolated combined-image fixture");
    auto combinedProcessor = std::make_unique<VDX7AudioProcessor>(false, folder);
    require(combinedProcessor->loadRomFromFile(combinedFile.getFile()), "valid legacy combined fixture");
    require(std::memcmp(ram(save(*combinedProcessor)).getData(), scan.banks.image.data(), 4096) == 0,
        "fresh combined-image working RAM uses overlaid ROM1A, not superseded legacy bytes");
    auto restored = std::make_unique<VDX7AudioProcessor>(false, folder);
    restored->setStateInformation(state.getData(), int(state.getSize()));
    require(restored->hasFactoryBank(2) && !restored->hasFactoryBank(0), "project catalog independent of absent local files");
    require(restored->getCurrentPatchName() == "LAST EDIT" && restored->getCurrentProgram() == 7,
        "last-edited patch name/program restored");
    require(sameProjectRam(editedRam, ram(save(*restored))), "complete latest patch persistent RAM restored");
    require(restored->setMonoCorrectionFromUi(true), "partial catalog MONO correction reboot");
    require(restored->hasFactoryBank(2) && !restored->hasFactoryBank(0)
        && sameProjectRam(editedRam, ram(save(*restored))), "MONO reboot preserves partial mask and edited sound");
    require(restored->setMonoCorrectionFromUi(false), "restore native MONO policy");
    const auto beforeInvalid = save(*restored);
    auto malformedXml = juce::AudioProcessor::getXmlFromBinary(state.getData(), int(state.getSize()));
    auto malformed = juce::ValueTree::fromXml(*malformedXml);
    malformed.setProperty("factoryBankMask", 256, nullptr);
    juce::MemoryBlock malformedState;
    juce::AudioProcessor::copyXmlToBinary(*malformed.createXml(), malformedState);
    restored->setStateInformation(malformedState.getData(), int(malformedState.getSize()));
    require(save(*restored) == beforeInvalid, "invalid catalog cannot replace loaded project");
    auto missing = juce::ValueTree::fromXml(*malformedXml);
    missing.setProperty("romPath", folder.getChildFile("missing.rom").getFullPathName(), nullptr);
    juce::MemoryBlock missingState;
    juce::AudioProcessor::copyXmlToBinary(*missing.createXml(), missingState);
    auto pending = std::make_unique<VDX7AudioProcessor>(false, folder);
    pending->setStateInformation(missingState.getData(), int(missingState.getSize()));
    require(!pending->isProjectReady(), "missing firmware leaves project pending");
    require(!pending->refreshFactoryBanks(folder, report), "refresh blocked during pending restore");
    const auto resavedMissing = save(*pending);
    pending = std::make_unique<VDX7AudioProcessor>(false, folder);
    pending->setStateInformation(resavedMissing.getData(), int(resavedMissing.getSize()));
    require(pending->loadRomFromFile(firmwareFile.getFile()), "matching firmware resolves pending catalog");
    require(pending->hasFactoryBank(2) && !pending->hasFactoryBank(0)
        && sameProjectRam(editedRam, ram(save(*pending))), "missing ROM resave preserves catalog and latest patch");
    require(restored->selectFactoryBank(2), "project's preserved factory bank remains usable");
    const auto selected = ram(save(*restored));
    require(std::memcmp(selected.getData(), scan.banks.image.data()+2*4096, 4096) == 0,
        "bank selection uses project's unchanged reference payload");
    require(restored->loadSyxFromFile(sourceFolder.getChildFile("rom1a.syx")), "normal SYX remains custom");
    require(restored->getCurrentBank() == -1, "normal import does not silently become factory selection");
    require(restored->renameVoice("CUSTOM NEW"), "new custom voice");
    const auto custom = save(*restored);
    auto customRestored = std::make_unique<VDX7AudioProcessor>(false);
    customRestored->setStateInformation(custom.getData(), int(custom.getSize()));
    require(customRestored->getCurrentPatchName() == "CUSTOM NEW"
        && sameProjectRam(ram(custom), ram(save(*customRestored))), "custom sound also recalled independently");
    std::cout << "PASS: private eight-bank identities, partial folder, last edits, absent-file catalog recall and custom recall\n";
}

int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    try
    {
        if (argc == 2 && juce::String(argv[1]) == "--pre-rom-state-state-only")
        {
            testPendingFactoryCatalog();
            testPreRomDeferredStateIsSerialized();
            testLegacyPendingProjectState();
            return 0;
        }
        if (argc == 4 && juce::String(argv[1]) == "--factory-bank-library")
        {
            testPendingFactoryCatalog();
            testFactoryBankFolder(juce::File(argv[2]), juce::File(argv[3]));
            return 0;
        }
        if (argc == 3 && juce::String(argv[1]) == "--legacy-bank-compatibility")
        {
            testLegacyPendingProjectState();
            testLegacyProcessorImport(juce::File(argv[2]));
            return 0;
        }
        if (argc == 3 && juce::String(argv[1]) == "--pre-rom-state")
        {
            testPreRomParameterEditsSurviveSave(juce::File(argv[2]));
            return 0;
        }
        auto originalStorage = std::make_unique<VDX7AudioProcessor>(false);
        auto& original = *originalStorage;
        {
            auto noRomStorage = std::make_unique<VDX7AudioProcessor>(false);
            auto& noRom = *noRomStorage;
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
        const juce::File testRomFile(explicitRom);
        if (explicitRom.isEmpty() || !testRomFile.existsAsFile())
        {
            std::cout << "SKIP: explicit user ROM required for processor integration tests\n";
            return 77;
        }
        require(original.loadRomFromFile(testRomFile), "explicit local test ROM");
        testPreRomParameterEditsSurviveSave(testRomFile);
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
        testLegacyProcessorImport(testRomFile);
        checkControllers(juce::File(original.getRomPath()));
        checkUserLibrary(juce::File(original.getRomPath()), argc > 1 ? juce::File(argv[1]) : juce::File());
        // Check patch/host coherence without creating an editor.
        original.selectProgramFromUi(3);
        auto state = save(original);
        auto bank = ram(state);
        // N3: a well-sized project RAM block can still contain invalid VMEM.
        // Probe an unselected voice so host parameter publication cannot repair it.
        for (const bool deferRom : {false, true})
        for (const bool legacy : {false, true})
        {
            auto probe = std::make_unique<VDX7AudioProcessor>(false);
            auto xml = juce::AudioProcessor::getXmlFromBinary(state.getData(), int(state.getSize()));
            auto tree = juce::ValueTree::fromXml(*xml);
            if (legacy) tree.removeAllChildren(nullptr);
            if (deferRom) tree.setProperty("romPath", "", nullptr);
            else require(probe->loadRomFromFile(testRomFile), "N3 loaded fixture");
            juce::MemoryBlock validState;
            juce::AudioProcessor::copyXmlToBinary(*tree.createXml(), validState);
            probe->setStateInformation(validState.getData(), int(validState.getSize()));
            const auto before = save(*probe);
            for (const auto mutation : std::array<std::pair<int, uint8_t>, 3>{{{0,100},{12,0x78},{117,49}}})
            {
                auto badRam = bank;
                static_cast<uint8_t*>(badRam.getData())[31 * 128 + mutation.first] = mutation.second;
                auto badTree = tree.createCopy();
                badTree.setProperty("ram", badRam.toBase64Encoding(), nullptr);
                badTree.setProperty("midiInputChannel", 9, nullptr);
                juce::MemoryBlock badState;
                juce::AudioProcessor::copyXmlToBinary(*badTree.createXml(), badState);
                probe->setStateInformation(badState.getData(), int(badState.getSize()));
                require(save(*probe) == before,
                        "invalid packed project RAM must preserve loaded or pending state transactionally");
            }
        }
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
        auto restoredStorage = std::make_unique<VDX7AudioProcessor>(false);
        auto& restored = *restoredStorage;
        restored.setStateInformation(state.getData(), int(state.getSize()));
        require(restored.getCurrentProgram() == 3, "program restore");
        const auto restoredRam = ram(save(restored));
        const auto savedRam = ram(state);
        checkProjectRamOracle(savedRam);
        require(sameProjectRam(savedRam, restoredRam), "persistent RAM round trip before rendering");
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
        // The round-trip oracle must describe bankFile, before the deliberate
        // edits below exercise immutable asynchronous export snapshots.
        const auto exportedBank = ram(save(restored));
        // Model the asynchronous file-chooser interval: capture first, then
        // apply a host automation edit through an audio callback before writing.
        VDX7AudioProcessor::SyxExportSnapshot patchSnapshot;
        require(restored.captureSyxExportSnapshot(false, patchSnapshot, error), "capture patch export snapshot");
        require(patchSnapshot.message().size() == VDX7Sysex::kVoiceMessageSize,
                "patch snapshot has one complete VCED message");
        const auto levelId = VDX7ParameterIDs::operatorParameter(0, VDX7VoiceData::Parameter::outputLevel);
        const auto levelBeforeEdit = juce::roundToInt(value(restored, levelId));
        set(restored, levelId, levelBeforeEdit == 0 ? 1 : levelBeforeEdit - 1);
        juce::AudioBuffer<float> exportAudio(2, 256);
        juce::MidiBuffer exportMidi;
        restored.prepareToPlay(48000, 256);
        restored.processBlock(exportAudio, exportMidi);
        require(restored.getCurrentProgram() == patchSnapshot.program(),
                "host edit leaves selected program unchanged during export dialog");
        VDX7AudioProcessor::SyxExportSnapshot editedPatch;
        require(restored.captureSyxExportSnapshot(false, editedPatch, error)
                && editedPatch.message() != patchSnapshot.message(),
                "host automation changes live patch after snapshot capture");
        juce::TemporaryFile snapshotPatchFile(".syx");
        require(restored.exportSyxSnapshot(snapshotPatchFile.getFile(), patchSnapshot, error),
                "write captured patch snapshot");
        juce::MemoryBlock writtenPatch;
        require(snapshotPatchFile.getFile().loadFileAsData(writtenPatch)
                && writtenPatch.getSize() == patchSnapshot.message().size()
                && std::memcmp(writtenPatch.getData(), patchSnapshot.message().data(), writtenPatch.getSize()) == 0,
                "patch export writes the pre-dialog snapshot, not live RAM");
        require(restored.hasUnexportedEdits(), "post-snapshot patch edit remains marked unexported");

        VDX7AudioProcessor::SyxExportSnapshot bankSnapshot;
        require(restored.captureSyxExportSnapshot(true, bankSnapshot, error), "capture bank export snapshot");
        require(bankSnapshot.message().size() == VDX7Sysex::kBankMessageSize,
                "bank snapshot has one complete VMEM message");
        const auto levelAfterPatchEdit = juce::roundToInt(value(restored, levelId));
        set(restored, levelId, levelAfterPatchEdit == 0 ? 1 : levelAfterPatchEdit - 1);
        exportAudio.clear(); exportMidi.clear();
        restored.processBlock(exportAudio, exportMidi);
        VDX7AudioProcessor::SyxExportSnapshot editedBank;
        require(restored.captureSyxExportSnapshot(true, editedBank, error)
                && editedBank.message() != bankSnapshot.message(),
                "host automation changes live bank after snapshot capture");
        juce::TemporaryFile snapshotBankFile(".syx");
        require(restored.exportSyxSnapshot(snapshotBankFile.getFile(), bankSnapshot, error),
                "write captured bank snapshot");
        juce::MemoryBlock writtenBank;
        require(snapshotBankFile.getFile().loadFileAsData(writtenBank)
                && writtenBank.getSize() == bankSnapshot.message().size()
                && std::memcmp(writtenBank.getData(), bankSnapshot.message().data(), writtenBank.getSize()) == 0,
                "bank export writes the pre-dialog snapshot, not live RAM");
        require(restored.hasUnexportedEdits(), "post-snapshot bank edit remains marked unexported");
        const auto afterSnapshotEdits = ram(save(restored));
        require(std::memcmp(exportedBank.getData(), afterSnapshotEdits.getData(), 4096) != 0,
                "snapshot edits must differ from the original exported bank");
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
                int operatorEnvelopeControls = 0;
                for (auto* child : editor->getChildren())
                    if (auto* slider = dynamic_cast<juce::Slider*>(child))
                        if (slider->getName().startsWith("Operator envelope "))
                        {
                            ++operatorEnvelopeControls;
                            require(slider->getHeight() >= 168 * scale,
                                    "operator envelope uses expanded vertical travel");
                            require(slider->getBottom() <= 796 * scale + 1,
                                    "operator envelope leaves room for value and keyboard");
                        }
                require(operatorEnvelopeControls == 8, "all eight expanded envelope controls");
                int pitchControls = 0;
                for (auto* child : editor->getChildren())
                    if (auto* slider = dynamic_cast<juce::Slider*>(child))
                        if (slider->getName().startsWith("Pitch envelope ")) {
                            ++pitchControls;
                            require(slider->getHeight() >= 73 * scale, "pitch faders taller");
                            // JUCE insets the drawing area by the thumb radius;
                            // the component/hit area extends below the artwork.
                            // Our fader renderer clamps the cap inside that area.
                            const auto drawingBounds = slider->getLookAndFeel()
                                .getSliderLayout(*slider).sliderBounds.translated(
                                    slider->getX(), slider->getY());
                            require(drawingBounds.getBottom() <= 488 * scale + 1,
                                    "pitch fader artwork leaves room for value labels");
                        }
                require(pitchControls == 8, "eight expanded pitch faders");
                const juce::Rectangle<float> lcdArea(422 * scale, 190 * scale,
                                                      505 * scale, 86 * scale);
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
                int headerButtons = 0;
                int headerBottom = -1;
                for (auto* child : editor->getChildren())
                {
                    if (auto* box = dynamic_cast<juce::ComboBox*>(child))
                        if (box->getName() == "Algorithm") algorithmBox = box;
                    if (auto* button = dynamic_cast<juce::TextButton*>(child))
                    {
                        if (bool(button->getProperties().getWithDefault("vdx7WideHeader", false)))
                        {
                            ++headerButtons;
                            if (headerBottom < 0) headerBottom = button->getBottom();
                            require(headerBottom == button->getBottom(), "header buttons share lower alignment");
                            require(button->getX() >= 893 * scale,
                                    "header actions leave branding area clear");
                        }
                        if (button->getButtonText() == "<") previous = button;
                        if (button->getButtonText() == ">") next = button;
                        if (button->getButtonText() == "UTILITY")
                            require(412 * scale - button->getRight() >= 27 * scale - 1,
                                    "UTILITY has clear gap before LCD frame");
                        if (button->getButtonText() == "SAVE AS...")
                            require(button->isEnabled(), "Save As enabled with ROM");
                    }
                    if (auto* label = dynamic_cast<juce::Label*>(child))
                        require(label->getText() != "ALGO", "duplicate algorithm encoder removed");
                }
                require(algorithmBox && algorithmBox->getNumItems() == 32,
                        "numbered 32-algorithm selector");
                require(headerButtons == 5, "all five header actions remain available");
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
        require(sameProjectRam(ram(state), ram(save(restored))), "RAM-only persistent state restoration");

        // Run a factory voice through real firmware without opening an audio device.
        auto renderStorage = std::make_unique<VDX7AudioProcessor>(false);
        auto& render = *renderStorage;
        require(render.loadRomFromFile(testRomFile), "explicit render test ROM");
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
