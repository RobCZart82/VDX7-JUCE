#include "PluginEditor.h"

VDX7AudioProcessorEditor::VDX7AudioProcessorEditor(VDX7AudioProcessor& p)
    : AudioProcessorEditor(&p), processor_(p)
{
    setSize(620, 330);

    title_.setText("VDX7 Mk I", juce::dontSendNotification);
    title_.setFont(juce::Font(juce::FontOptions(26.0f, juce::Font::bold)));
    title_.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(title_);

    subtitle_.setText("GPL prototype - VDX7 hardware emulation core / user-supplied Yamaha firmware", juce::dontSendNotification);
    subtitle_.setFont(juce::Font(juce::FontOptions(12.0f)));
    subtitle_.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(subtitle_);

    status_.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(status_);

    romPath_.setJustificationType(juce::Justification::centredLeft);
    romPath_.setMinimumHorizontalScale(0.6f);
    addAndMakeVisible(romPath_);

    patch_.setJustificationType(juce::Justification::centred);
    patch_.setFont(juce::Font(juce::FontOptions(24.0f, juce::Font::bold)));
    addAndMakeVisible(patch_);

    bankCaption_.setText("Factory bank", juce::dontSendNotification);
    programCaption_.setText("Program", juce::dontSendNotification);
    addAndMakeVisible(bankCaption_);
    addAndMakeVisible(programCaption_);

    static constexpr const char* bankNames[] =
        { "ROM1A", "ROM1B", "ROM2A", "ROM2B", "ROM3A", "ROM3B", "ROM4A", "ROM4B" };
    for (int i = 0; i < 8; ++i)
        bank_.addItem(bankNames[i], i + 1);
    addAndMakeVisible(bank_);

    for (int i = 0; i < 32; ++i)
        program_.addItem(juce::String(i + 1).paddedLeft('0', 2), i + 1);
    addAndMakeVisible(program_);

    addAndMakeVisible(loadRom_);
    addAndMakeVisible(loadSyx_);
    addAndMakeVisible(previous_);
    addAndMakeVisible(next_);

    loadRom_.onClick = [this] { chooseRom(); };
    loadSyx_.onClick = [this] { chooseSyx(); };

    bank_.onChange = [this]
    {
        if (internalUiUpdate_) return;
        const int bankIndex = bank_.getSelectedId() - 1;
        if (bankIndex >= 0)
            processor_.selectFactoryBank(bankIndex);
    };

    program_.onChange = [this]
    {
        if (internalUiUpdate_) return;
        const int programIndex = program_.getSelectedId() - 1;
        if (programIndex >= 0)
            processor_.selectProgramFromUi(programIndex);
    };

    previous_.onClick = [this]
    {
        const int current = processor_.getCurrentProgram();
        processor_.selectProgramFromUi((current + 31) % 32);
        refresh();
    };

    next_.onClick = [this]
    {
        const int current = processor_.getCurrentProgram();
        processor_.selectProgramFromUi((current + 1) % 32);
        refresh();
    };

    startTimerHz(5);
    refresh();
}

void VDX7AudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff202124));

    auto display = getLocalBounds().reduced(18).withTrimmedTop(105).withHeight(72);
    g.setColour(juce::Colour(0xff101514));
    g.fillRoundedRectangle(display.toFloat(), 6.0f);
    g.setColour(juce::Colour(0xff6f8f72));
    g.drawRoundedRectangle(display.toFloat(), 6.0f, 1.5f);
}

void VDX7AudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(18);
    title_.setBounds(area.removeFromTop(34));
    subtitle_.setBounds(area.removeFromTop(22));

    auto buttons = area.removeFromTop(38);
    loadRom_.setBounds(buttons.removeFromLeft(130).reduced(2));
    loadSyx_.setBounds(buttons.removeFromLeft(130).reduced(2));
    status_.setBounds(buttons.reduced(6, 2));

    romPath_.setBounds(area.removeFromTop(28));

    auto display = area.removeFromTop(72).reduced(6);
    patch_.setBounds(display);

    area.removeFromTop(8);
    auto controls = area.removeFromTop(62);

    auto bankArea = controls.removeFromLeft(180);
    bankCaption_.setBounds(bankArea.removeFromTop(20));
    bank_.setBounds(bankArea.removeFromTop(32));

    controls.removeFromLeft(16);
    auto progArea = controls.removeFromLeft(220);
    programCaption_.setBounds(progArea.removeFromTop(20));
    auto progRow = progArea.removeFromTop(32);
    previous_.setBounds(progRow.removeFromLeft(42));
    program_.setBounds(progRow.removeFromLeft(120).reduced(4, 0));
    next_.setBounds(progRow.removeFromLeft(42));
}

void VDX7AudioProcessorEditor::timerCallback()
{
    refresh();
}

void VDX7AudioProcessorEditor::refresh()
{
    internalUiUpdate_ = true;

    const bool loaded = processor_.isRomLoaded();
    const bool factories = processor_.hasFactoryVoices();

    status_.setText(processor_.getStatusText(), juce::dontSendNotification);
    romPath_.setText(processor_.getRomPath(), juce::dontSendNotification);
    patch_.setText(loaded ? processor_.getCurrentPatchName() : "LOAD DX7 ROM", juce::dontSendNotification);

    const int bankIndex = processor_.getCurrentBank();
    bank_.setSelectedId(bankIndex >= 0 ? bankIndex + 1 : 0, juce::dontSendNotification);
    program_.setSelectedId(processor_.getCurrentProgram() + 1, juce::dontSendNotification);

    bank_.setEnabled(loaded && factories);
    program_.setEnabled(loaded);
    loadSyx_.setEnabled(loaded);
    previous_.setEnabled(loaded);
    next_.setEnabled(loaded);

    internalUiUpdate_ = false;
}

void VDX7AudioProcessorEditor::chooseRom()
{
    chooser_ = std::make_unique<juce::FileChooser>(
        "Select Yamaha DX7 Mk I firmware",
        processor_.getSuggestedRomFolder(),
        "*.bin;*.BIN;*.obj;*.OBJ");

    chooser_->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc)
        {
            const auto file = fc.getResult();
            if (!file.existsAsFile()) return;
            juce::String error;
            if (!processor_.loadRomFromFile(file, &error))
                showError("VDX7 ROM error", error);
            refresh();
        });
}

void VDX7AudioProcessorEditor::chooseSyx()
{
    chooser_ = std::make_unique<juce::FileChooser>(
        "Select a Yamaha DX7 32-voice SysEx bank",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
        "*.syx;*.SYX");

    chooser_->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc)
        {
            const auto file = fc.getResult();
            if (!file.existsAsFile()) return;
            juce::String error;
            if (!processor_.loadSyxFromFile(file, &error))
                showError("VDX7 SysEx error", error);
            refresh();
        });
}

void VDX7AudioProcessorEditor::showError(const juce::String& title, const juce::String& message)
{
    juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, title, message);
}
