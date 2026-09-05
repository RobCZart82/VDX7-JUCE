#include "PluginEditor.h"

#include <BinaryData.h>

#include <array>
#include <cmath>

namespace
{
constexpr float kReferenceWidth = 1440.0f;
constexpr float kReferenceHeight = 1110.0f;
constexpr int kTabGroup = 1001;
constexpr int kOperatorTabGroup = 1002;
constexpr double kCpuDisplaySmoothing = 0.015;
constexpr int kCpuTextRefreshFrames = 15;

void drawEnvelopeGrid(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    g.setColour(juce::Colour(0xff031b20));
    g.fillRoundedRectangle(bounds, 3.0f);
    const auto grid = bounds.reduced(1.0f);
    g.setColour(juce::Colour(0xff0b3740));
    for (int i = 1; i < 10; ++i)
    {
        const float x = grid.getX() + grid.getWidth() * i / 10.0f;
        g.drawLine(x, grid.getY(), x, grid.getBottom(), 0.6f);
    }
    for (int i = 1; i < 6; ++i)
    {
        const float y = grid.getY() + grid.getHeight() * i / 6.0f;
        g.drawLine(grid.getX(), y, grid.getRight(), y, 0.6f);
    }
}

constexpr std::array<VDX7VoiceData::Parameter, 7> kKnobParameters
{
    VDX7VoiceData::Parameter::outputLevel,
    VDX7VoiceData::Parameter::coarse,
    VDX7VoiceData::Parameter::fine,
    VDX7VoiceData::Parameter::detune,
    VDX7VoiceData::Parameter::rateScaling,
    VDX7VoiceData::Parameter::velocitySensitivity,
    VDX7VoiceData::Parameter::amplitudeModSensitivity
};

constexpr std::array<const char*, 7> kKnobCaptions
{
    "OUTPUT", "COARSE", "FINE", "DETUNE", "RATE SCALE", "VEL SENS", "AMP MOD"
};

constexpr std::array<VDX7VoiceData::Parameter, 8> kEnvelopeParameters
{
    VDX7VoiceData::Parameter::rate1,
    VDX7VoiceData::Parameter::rate2,
    VDX7VoiceData::Parameter::rate3,
    VDX7VoiceData::Parameter::rate4,
    VDX7VoiceData::Parameter::level1,
    VDX7VoiceData::Parameter::level2,
    VDX7VoiceData::Parameter::level3,
    VDX7VoiceData::Parameter::level4
};

constexpr std::array<const char*, 8> kEnvelopeCaptions
{
    "R1", "R2", "R3", "R4", "L1", "L2", "L3", "L4"
};

constexpr std::array<VDX7VoiceData::Parameter, 6> kScaleParameters
{
    VDX7VoiceData::Parameter::oscillatorMode,
    VDX7VoiceData::Parameter::breakpoint,
    VDX7VoiceData::Parameter::leftScaleDepth,
    VDX7VoiceData::Parameter::leftScaleCurve,
    VDX7VoiceData::Parameter::rightScaleDepth,
    VDX7VoiceData::Parameter::rightScaleCurve
};

constexpr std::array<const char*, 6> kScaleCaptions
{
    "OSC MODE", "BREAKPOINT", "LEFT DEPTH", "LEFT CURVE", "RIGHT DEPTH", "RIGHT CURVE"
};

constexpr std::array<VDX7VoiceData::VoiceParameter, 8> kPitchEnvelopeParameters
{
    VDX7VoiceData::VoiceParameter::pitchRate1,
    VDX7VoiceData::VoiceParameter::pitchRate2,
    VDX7VoiceData::VoiceParameter::pitchRate3,
    VDX7VoiceData::VoiceParameter::pitchRate4,
    VDX7VoiceData::VoiceParameter::pitchLevel1,
    VDX7VoiceData::VoiceParameter::pitchLevel2,
    VDX7VoiceData::VoiceParameter::pitchLevel3,
    VDX7VoiceData::VoiceParameter::pitchLevel4
};

constexpr std::array<VDX7VoiceData::VoiceParameter, 11> kVoiceKnobParameters
{
    VDX7VoiceData::VoiceParameter::algorithm,
    VDX7VoiceData::VoiceParameter::feedback,
    VDX7VoiceData::VoiceParameter::oscillatorKeySync,
    VDX7VoiceData::VoiceParameter::transpose,
    VDX7VoiceData::VoiceParameter::lfoSpeed,
    VDX7VoiceData::VoiceParameter::lfoDelay,
    VDX7VoiceData::VoiceParameter::pitchModDepth,
    VDX7VoiceData::VoiceParameter::amplitudeModDepth,
    VDX7VoiceData::VoiceParameter::lfoKeySync,
    VDX7VoiceData::VoiceParameter::lfoWaveform,
    VDX7VoiceData::VoiceParameter::pitchModSensitivity
};

constexpr std::array<const char*, 11> kVoiceKnobCaptions
{
    "ALGO", "FDBK", "O-SYNC", "TRANS", "SPEED", "DELAY",
    "PMD", "AMD", "L-SYNC", "WAVE", "PMS"
};

juce::String breakpointName(int value)
{
    static constexpr const char* notes[]
        { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    const int midiNote = juce::jlimit(0, 99, value) + 9;
    return juce::String(notes[midiNote % 12]) + juce::String(midiNote / 12 - 1);
}

juce::String curveName(int value)
{
    static constexpr const char* curves[] { "-LIN", "-EXP", "+EXP", "+LIN" };
    return curves[juce::jlimit(0, 3, value)];
}

juce::String waveformName(int value)
{
    static constexpr const char* waves[] { "TRI", "SAW DN", "SAW UP", "SQUARE", "SINE", "S&H" };
    return waves[juce::jlimit(0, 5, value)];
}

juce::Image loadImage(const char* data, int size)
{
    return juce::ImageFileFormat::loadFrom(data, static_cast<std::size_t>(size));
}
}

VDX7Keyboard::VDX7Keyboard(juce::MidiKeyboardState& state)
    : juce::MidiKeyboardComponent(state, juce::MidiKeyboardComponent::horizontalKeyboard),
      whiteNormal_(loadImage(VDX7Assets::whitekeynormal_png, VDX7Assets::whitekeynormal_pngSize)),
      whitePressed_(loadImage(VDX7Assets::whitekeypressed_png, VDX7Assets::whitekeypressed_pngSize)),
      blackNormal_(loadImage(VDX7Assets::blackkeynormal_png, VDX7Assets::blackkeynormal_pngSize)),
      blackPressed_(loadImage(VDX7Assets::blackkeypressed_png, VDX7Assets::blackkeypressed_pngSize))
{
    setAvailableRange(36, 96);
    setLowestVisibleKey(36);
    setScrollButtonsVisible(false);
    setVelocity(0.82f, true);
    setWantsKeyboardFocus(false);
    setColour(mouseOverKeyOverlayColourId, juce::Colour(0x2400e7e7));
    setColour(keyDownOverlayColourId, juce::Colours::transparentBlack);
    setColour(whiteNoteColourId, juce::Colour(0xff071014));
    setBlackNoteLengthProportion(84.0f / 132.0f);
    setBlackNoteWidthProportion(24.0f / 36.0f);
}

void VDX7Keyboard::paintOverChildren(juce::Graphics& g)
{
    // A stationary felt strip above the keys, including pressed/hovered notes.
    const float thickness = juce::jmax(1.0f, getHeight() * 2.0f / 138.0f);
    g.setColour(juce::Colour(0xff8e1728));
    g.fillRect(0.0f, 0.0f, float(getWidth()), thickness);
    g.setColour(juce::Colour(0xffc4454f));
    g.fillRect(0.0f, 0.0f, float(getWidth()), thickness * 0.35f);
}

void VDX7Keyboard::drawWhiteNote(int, juce::Graphics& g, juce::Rectangle<float> area,
                                  bool isDown, bool isOver, juce::Colour, juce::Colour)
{
    g.setColour(juce::Colour(0xffeeeae2));
    g.fillRoundedRectangle(area.reduced(1.0f, area.getHeight() * 0.09f), 1.5f);
    g.drawImage(isDown ? whitePressed_ : whiteNormal_, area);
    if (isOver && !isDown)
    {
        g.setColour(juce::Colour(0x2600e7e7));
        g.fillRect(area.reduced(1.0f));
    }
}

void VDX7Keyboard::drawBlackNote(int, juce::Graphics& g, juce::Rectangle<float> area,
                                  bool isDown, bool isOver, juce::Colour)
{
    g.setColour(juce::Colour(0xff080b0d));
    g.fillRect(area);
    g.drawImage(isDown ? blackPressed_ : blackNormal_, area);
    if (!isDown)
    {
        g.setColour(juce::Colour(0x52000000));
        g.fillRoundedRectangle(area.reduced(1.0f), 1.5f);
    }
    if (isOver && !isDown)
    {
        g.setColour(juce::Colour(0x3000e7e7));
        g.fillRect(area.reduced(1.0f));
    }
}

VDX7WheelSlider::VDX7WheelSlider(bool springToCentre)
    : springToCentre_(springToCentre)
{
    setSliderStyle(juce::Slider::LinearVertical);
    setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    setSliderSnapsToMousePosition(false);
    setMouseDragSensitivity(180);
}

void VDX7WheelSlider::mouseUp(const juce::MouseEvent& event)
{
    juce::Slider::mouseUp(event);
    if (springToCentre_)
        setValue(0.0, juce::sendNotificationSync);
}

VDX7LevelMeter::VDX7LevelMeter()
    : rail_(loadImage(VDX7Assets::vurail_png, VDX7Assets::vurail_pngSize)),
      ledOff_(loadImage(VDX7Assets::ledoff_png, VDX7Assets::ledoff_pngSize)),
      ledGreen_(loadImage(VDX7Assets::ledgreen_png, VDX7Assets::ledgreen_pngSize)),
      ledYellow_(loadImage(VDX7Assets::ledyellow_png, VDX7Assets::ledyellow_pngSize)),
      ledRed_(loadImage(VDX7Assets::ledred_png, VDX7Assets::ledred_pngSize))
{
    setInterceptsMouseClicks(false, false);
}

void VDX7LevelMeter::setLevel(float newLevel)
{
    const float clipped = juce::jlimit(0.0f, 1.5f, newLevel);
    if (std::abs(clipped - level_) > 0.001f)
    {
        level_ = clipped;
        repaint();
    }
}

void VDX7LevelMeter::paint(juce::Graphics& g)
{
    constexpr int segmentCount = 12;
    const auto bounds = getLocalBounds().toFloat();
    g.drawImage(rail_, bounds);

    const float db = juce::Decibels::gainToDecibels(level_, -60.0f);
    const int litSegments = juce::jlimit(0, segmentCount,
        static_cast<int>(std::ceil((db + 60.0f) / 60.0f * segmentCount)));
    const float ledWidth = bounds.getWidth() * 0.50f;
    const float gap = juce::jmax(1.0f, bounds.getHeight() * 0.012f);
    const float ledHeight = (bounds.getHeight() - gap * (segmentCount + 1)) / segmentCount;

    for (int segment = 0; segment < segmentCount; ++segment)
    {
        const float y = bounds.getBottom() - gap - (segment + 1) * ledHeight - segment * gap;
        const auto ledBounds = juce::Rectangle<float>(ledWidth, ledHeight)
                                   .withCentre({ bounds.getCentreX(), y + ledHeight * 0.5f });

        const juce::Image* image = &ledOff_;
        if (segment < litSegments)
            image = segment >= 10 ? &ledRed_ : (segment >= 8 ? &ledYellow_ : &ledGreen_);
        g.drawImage(*image, ledBounds);
    }
}

VDX7AudioProcessorEditor::VDX7AudioProcessorEditor(VDX7AudioProcessor& processor)
    : AudioProcessorEditor(&processor),
      processor_(processor),
      keyboard_(processor.keyboardState()),
      chassis_(loadImage(VDX7Assets::mainwindow_png, VDX7Assets::mainwindow_pngSize)),
      wordmark_(loadImage(VDX7Assets::vdx7wordmark_png, VDX7Assets::vdx7wordmark_pngSize)),
      lcdFrame_(loadImage(VDX7Assets::lcdframe_png, VDX7Assets::lcdframe_pngSize)),
      panel_(loadImage(VDX7Assets::panel9slice_png, VDX7Assets::panel9slice_pngSize)),
      valueField_(loadImage(VDX7Assets::valuefield_png, VDX7Assets::valuefield_pngSize)),
      envelopeGrid_(loadImage(VDX7Assets::envelopegrid_png, VDX7Assets::envelopegrid_pngSize)),
      divider_(loadImage(VDX7Assets::sectiondivider_png, VDX7Assets::sectiondivider_pngSize))
{
    setLookAndFeel(&lookAndFeel_);
    setOpaque(true);
    setResizable(true, true);
    setResizeLimits(960, 740, 1600, 1234);
    if (auto* constrainer = getConstrainer())
        constrainer->setFixedAspectRatio(kReferenceWidth / kReferenceHeight);
    setSize(1200, 925);

    configureLabel(mkLabel_, 37.0f, juce::Justification::centredLeft);
    configureLabel(hardwareLabel_, 16.0f, juce::Justification::centredLeft);
    configureLabel(firmwareLabel_, 13.0f, juce::Justification::centredLeft, juce::Colour(0xff91a5ac));
    configureLabel(headerPatch_, 18.0f, juce::Justification::centred, juce::Colour(0xff00e7e7));
    configureLabel(status_, 12.0f, juce::Justification::centredLeft, juce::Colour(0xff91a5ac));
    configureLabel(patch_, 29.0f, juce::Justification::centredLeft, juce::Colour(0xff06352e));
    configureLabel(bankCaption_, 12.0f, juce::Justification::centredLeft, juce::Colour(0xff06352e));
    configureLabel(programCaption_, 12.0f, juce::Justification::centredLeft, juce::Colour(0xff06352e));
    configureLabel(masterCaption_, 14.0f, juce::Justification::centred);
    configureLabel(masterValue_, 17.0f, juce::Justification::centred, juce::Colour(0xff00e7e7));
    configureLabel(pitchCaption_, 14.0f, juce::Justification::centred);
    configureLabel(modCaption_, 14.0f, juce::Justification::centred);
    configureLabel(outputCaption_, 19.0f, juce::Justification::centred);
    configureLabel(leftCaption_, 12.0f, juce::Justification::centred);
    configureLabel(rightCaption_, 12.0f, juce::Justification::centred);
    configureLabel(algorithmDisplay_, 30.0f, juce::Justification::centred,
                   juce::Colour(0xff00e7e7));
    configureLabel(operatorTitle_, 20.0f, juce::Justification::centredLeft);
    configureLabel(frequencyValue_, 14.0f, juce::Justification::centredRight,
                   juce::Colour(0xff00e7e7));
    configureLabel(envelopeTitle_, 13.0f, juce::Justification::centredLeft,
                   juce::Colour(0xff91a5ac));
    configureLabel(pitchEnvelopeTitle_, 12.0f, juce::Justification::centredLeft,
                   juce::Colour(0xff91a5ac));
    configureLabel(voiceLfoTitle_, 12.0f, juce::Justification::centredLeft,
                   juce::Colour(0xff91a5ac));
    configureLabel(footerLeft_, 15.0f, juce::Justification::centredLeft);
    configureLabel(footerCentre_, 15.0f, juce::Justification::centred, juce::Colour(0xff62767d));
    configureLabel(footerRight_, 12.0f, juce::Justification::centredRight, juce::Colour(0xff91a5ac));

    mkLabel_.setText("Mk I.", juce::dontSendNotification);
    hardwareLabel_.setText("HARDWARE EMULATION", juce::dontSendNotification);
    firmwareLabel_.setText("Original firmware required", juce::dontSendNotification);
    masterCaption_.setText("VOLUME", juce::dontSendNotification);
    pitchCaption_.setText("PITCH", juce::dontSendNotification);
    modCaption_.setText("MOD", juce::dontSendNotification);
    outputCaption_.setText("OUTPUT", juce::dontSendNotification);
    leftCaption_.setText("LEFT", juce::dontSendNotification);
    rightCaption_.setText("RIGHT", juce::dontSendNotification);
    algorithmDisplay_.setText("1", juce::dontSendNotification);
    operatorTitle_.setText("OPERATOR 1", juce::dontSendNotification);
    frequencyValue_.setText("RATIO 1.00 x", juce::dontSendNotification);
    envelopeTitle_.setText("4-STAGE ENVELOPE", juce::dontSendNotification);
    pitchEnvelopeTitle_.setText("PITCH ENVELOPE", juce::dontSendNotification);
    voiceLfoTitle_.setText("VOICE + LFO", juce::dontSendNotification);
    footerLeft_.setText("VDX7 Mk I     v0.6.6", juce::dontSendNotification);
    footerCentre_.setText("6-OPERATOR FM SYNTHESIZER", juce::dontSendNotification);
    footerRight_.setText("CPU 0.0%", juce::dontSendNotification);

    headerPatch_.setColour(juce::Label::backgroundColourId, juce::Colour(0xff050b0d));
    headerPatch_.setColour(juce::Label::outlineColourId, juce::Colour(0xff24444b));

    for (auto* label : { &mkLabel_, &hardwareLabel_, &firmwareLabel_, &headerPatch_, &status_,
                         &patch_, &bankCaption_, &programCaption_, &masterCaption_,
                         &masterValue_, &pitchCaption_, &modCaption_, &outputCaption_, &leftCaption_,
                         &rightCaption_, &algorithmDisplay_, &operatorTitle_, &frequencyValue_,
                         &envelopeTitle_, &pitchEnvelopeTitle_, &voiceLfoTitle_, &footerLeft_,
                         &footerCentre_, &footerRight_ })
        addAndMakeVisible(*label);

    for (auto* button : { &loadRom_, &loadSyx_, &settings_, &about_, &previous_, &next_,
                          &editTab_, &performanceTab_, &utilityTab_ })
        addAndMakeVisible(*button);

    configureTab(editTab_, kTabGroup);
    configureTab(performanceTab_, kTabGroup);
    configureTab(utilityTab_, kTabGroup);
    editTab_.setToggleState(true, juce::dontSendNotification);

    for (int op = 0; op < VDX7VoiceData::kOperatorCount; ++op)
    {
        auto& tab = operatorTabs_[static_cast<std::size_t>(op)];
        tab.setButtonText("OP" + juce::String(op + 1));
        configureTab(tab, kOperatorTabGroup);
        tab.onClick = [this, op] { selectOperator(op); };
        addAndMakeVisible(tab);
    }

    addAndMakeVisible(algorithmView_);
    algorithmView_.onOperatorSelected = [this](int op) { selectOperator(op); };
    settings_.setEnabled(false);
    settings_.setTooltip("The settings page will be connected in the next GUI milestone.");
    performanceTab_.setEnabled(false);
    performanceTab_.setTooltip("Performance controls will be expanded in a later milestone.");
    utilityTab_.setClickingTogglesState(false);
    utilityTab_.setRadioGroupId(0);
    utilityTab_.setTooltip("Rename, save voice/bank and copy/paste the selected operator.");
    utilityTab_.onClick = [this] { showUtilityMenu(); };

    static constexpr const char* bankNames[] =
        { "ROM1A", "ROM1B", "ROM2A", "ROM2B", "ROM3A", "ROM3B", "ROM4A", "ROM4B" };
    for (int i = 0; i < 8; ++i)
        bank_.addItem(bankNames[i], i + 1);

    for (int i = 0; i < 32; ++i)
        program_.addItem(juce::String(i + 1).paddedLeft('0', 2), i + 1);

    addAndMakeVisible(bank_);
    addAndMakeVisible(program_);

    masterVolume_.setSliderStyle(juce::Slider::LinearVertical);
    masterVolume_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    masterVolume_.getProperties().set("vdx7Fader", true);
    masterVolume_.getProperties().set("vdx7MasterFader", true);
    masterVolume_.setDoubleClickReturnValue(true, 0.0);
    masterVolume_.setSliderSnapsToMousePosition(false);
    masterVolume_.setMouseDragSensitivity(260);

    addAndMakeVisible(masterVolume_);
    addAndMakeVisible(pitchWheel_);
    addAndMakeVisible(modWheel_);
    addAndMakeVisible(leftMeter_);
    addAndMakeVisible(rightMeter_);
    addAndMakeVisible(keyboard_);

    for (std::size_t i = 0; i < operatorKnobs_.size(); ++i)
    {
        configureOperatorSlider(operatorKnobs_[i], false);
        configureLabel(operatorKnobCaptions_[i], 11.0f, juce::Justification::centred,
                       juce::Colour(0xff91a5ac));
        configureLabel(operatorKnobValues_[i], 12.0f, juce::Justification::centred,
                       juce::Colour(0xff00e7e7));
        operatorKnobCaptions_[i].setText(kKnobCaptions[i], juce::dontSendNotification);
        operatorKnobValues_[i].setColour(juce::Label::backgroundColourId,
                                         juce::Colour(0xff071014));
        operatorKnobValues_[i].setColour(juce::Label::outlineColourId,
                                         juce::Colour(0xff24444b));
        operatorKnobs_[i].onValueChange = [this] { updateOperatorValueLabels(); };
        addAndMakeVisible(operatorKnobs_[i]);
        addAndMakeVisible(operatorKnobCaptions_[i]);
        addAndMakeVisible(operatorKnobValues_[i]);
    }

    for (std::size_t i = 0; i < envelopeFaders_.size(); ++i)
    {
        configureOperatorSlider(envelopeFaders_[i], true);
        configureLabel(envelopeCaptions_[i], 11.0f, juce::Justification::centred,
                       juce::Colour(0xff91a5ac));
        configureLabel(envelopeValues_[i], 11.0f, juce::Justification::centred,
                       juce::Colour(0xff00e7e7));
        envelopeCaptions_[i].setText(kEnvelopeCaptions[i], juce::dontSendNotification);
        envelopeValues_[i].setColour(juce::Label::backgroundColourId,
                                     juce::Colour(0xff071014));
        envelopeValues_[i].setColour(juce::Label::outlineColourId,
                                     juce::Colour(0xff24444b));
        envelopeFaders_[i].onValueChange = [this]
        {
            updateOperatorValueLabels();
            repaint(referenceRect(1080, 578, 312, 174));
        };
        addAndMakeVisible(envelopeFaders_[i]);
        addAndMakeVisible(envelopeCaptions_[i]);
        addAndMakeVisible(envelopeValues_[i]);
    }

    for (std::size_t i = 0; i < operatorScaleKnobs_.size(); ++i)
    {
        configureOperatorSlider(operatorScaleKnobs_[i], false);
        if (i == 0)
        {
            operatorScaleKnobs_[i].setSliderStyle(juce::Slider::LinearHorizontal);
            operatorScaleKnobs_[i].setSliderSnapsToMousePosition(true);
            operatorScaleKnobs_[i].getProperties().set("vdx7ModeSwitch", true);
        }
        configureLabel(operatorScaleCaptions_[i], 9.0f, juce::Justification::centred,
                       juce::Colour(0xff91a5ac));
        configureLabel(operatorScaleValues_[i], 10.0f, juce::Justification::centred,
                       juce::Colour(0xff00e7e7));
        operatorScaleCaptions_[i].setText(kScaleCaptions[i], juce::dontSendNotification);
        operatorScaleValues_[i].setColour(juce::Label::backgroundColourId,
                                          juce::Colour(0xff071014));
        operatorScaleValues_[i].setColour(juce::Label::outlineColourId,
                                          juce::Colour(0xff24444b));
        operatorScaleKnobs_[i].onValueChange = [this] { updateOperatorValueLabels(); };
        addAndMakeVisible(operatorScaleKnobs_[i]);
        addAndMakeVisible(operatorScaleCaptions_[i]);
        addAndMakeVisible(operatorScaleValues_[i]);
    }
    operatorScaleValues_[0].setVisible(false);
    frequencyValue_.setJustificationType(juce::Justification::centred);
    frequencyValue_.setBorderSize(juce::BorderSize<int>(0));
    frequencyValue_.setColour(juce::Label::backgroundColourId, juce::Colour(0xff071014));
    frequencyValue_.setColour(juce::Label::outlineColourId, juce::Colour(0xff24444b));

    for (std::size_t i = 0; i < pitchEnvelopeFaders_.size(); ++i)
    {
        configureOperatorSlider(pitchEnvelopeFaders_[i], true);
        configureLabel(pitchEnvelopeCaptions_[i], 9.0f, juce::Justification::centred,
                       juce::Colour(0xff91a5ac));
        configureLabel(pitchEnvelopeValues_[i], 9.0f, juce::Justification::centred,
                       juce::Colour(0xff00e7e7));
        pitchEnvelopeCaptions_[i].setText(kEnvelopeCaptions[i], juce::dontSendNotification);
        pitchEnvelopeValues_[i].setColour(juce::Label::backgroundColourId,
                                          juce::Colour(0xff071014));
        pitchEnvelopeValues_[i].setColour(juce::Label::outlineColourId,
                                          juce::Colour(0xff24444b));
        pitchEnvelopeFaders_[i].onValueChange = [this]
        {
            updateVoiceValueLabels();
            repaint(referenceRect(326, 387, 264, 119));
        };
        addAndMakeVisible(pitchEnvelopeFaders_[i]);
        addAndMakeVisible(pitchEnvelopeCaptions_[i]);
        addAndMakeVisible(pitchEnvelopeValues_[i]);
    }

    for (std::size_t i = 0; i < voiceKnobs_.size(); ++i)
    {
        configureOperatorSlider(voiceKnobs_[i], false);
        if (i == 2 || i == 8)
        {
            voiceKnobs_[i].setSliderStyle(juce::Slider::LinearVertical);
            voiceKnobs_[i].setSliderSnapsToMousePosition(true);
            voiceKnobs_[i].getProperties().set("vdx7SyncSwitch", true);
        }
        configureLabel(voiceKnobCaptions_[i], 8.5f, juce::Justification::centred,
                       juce::Colour(0xff91a5ac));
        configureLabel(voiceKnobValues_[i], 9.0f, juce::Justification::centred,
                       juce::Colour(0xff00e7e7));
        voiceKnobCaptions_[i].setText(kVoiceKnobCaptions[i], juce::dontSendNotification);
        voiceKnobCaptions_[i].setBorderSize(juce::BorderSize<int>(0));
        voiceKnobValues_[i].setColour(juce::Label::backgroundColourId,
                                      juce::Colour(0xff071014));
        voiceKnobValues_[i].setColour(juce::Label::outlineColourId,
                                      juce::Colour(0xff24444b));
        voiceKnobs_[i].onValueChange = [this] { updateVoiceValueLabels(); };
        addAndMakeVisible(voiceKnobs_[i]);
        addAndMakeVisible(voiceKnobCaptions_[i]);
        addAndMakeVisible(voiceKnobValues_[i]);
    }

    auto& parameters = processor_.parameters();
    masterVolumeAttachment_ = std::make_unique<SliderAttachment>(
        parameters, VDX7ParameterIDs::masterVolume, masterVolume_);
    pitchWheelAttachment_ = std::make_unique<SliderAttachment>(
        parameters, VDX7ParameterIDs::pitchWheel, pitchWheel_);
    modWheelAttachment_ = std::make_unique<SliderAttachment>(
        parameters, VDX7ParameterIDs::modWheel, modWheel_);

    for (std::size_t i = 0; i < pitchEnvelopeAttachments_.size(); ++i)
        pitchEnvelopeAttachments_[i] = std::make_unique<SliderAttachment>(
            parameters, VDX7ParameterIDs::voiceParameter(kPitchEnvelopeParameters[i]),
            pitchEnvelopeFaders_[i]);

    for (std::size_t i = 0; i < voiceKnobAttachments_.size(); ++i)
        voiceKnobAttachments_[i] = std::make_unique<SliderAttachment>(
            parameters, VDX7ParameterIDs::voiceParameter(kVoiceKnobParameters[i]),
            voiceKnobs_[i]);

    masterVolume_.onValueChange = [this]
    {
        masterValue_.setText(juce::String(masterVolume_.getValue(), 1) + " dB",
                             juce::dontSendNotification);
    };

    loadRom_.onClick = [this] { chooseRom(); };
    loadSyx_.onClick = [this] { chooseSyx(); };
    about_.onClick = [this]
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::MessageBoxIconType::InfoIcon,
            "About VDX7 Mk I",
            "VDX7 Mk I v0.6.6\nOscillator switch, subtle hover and full envelope grids\n\n"
            "VDX7-JUCE: GNU AGPLv3, without warranty.\n"
            "DX7 core: chiaccona / Retromulator, GPLv3-or-later.\n"
            "JUCE: AGPLv3. Source and license notices:\n"
            "https://github.com/RobCZart82/VDX7-JUCE\n\n"
            "No Yamaha logo or firmware is included. A user-supplied compatible ROM is required.");
    };

    bankCaption_.setText("Bank:", juce::dontSendNotification);
    programCaption_.setText("Program:", juce::dontSendNotification);
    for (auto* box : { &bank_, &program_ })
    {
        box->getProperties().set("vdx7LcdCombo", true);
        box->setColour(juce::ComboBox::backgroundColourId, juce::Colours::transparentBlack);
        box->setColour(juce::ComboBox::textColourId, juce::Colour(0xff06352e));
        box->setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff06352e));
        box->setColour(juce::ComboBox::arrowColourId, juce::Colour(0xff06352e));
    }
    bank_.onChange = [this]
    {
        if (internalUiUpdate_) return;
        const int bankIndex = bank_.getSelectedId() - 1;
        if (bankIndex >= 0)
        {
            refresh(false);
            confirmReplacement([this, bankIndex] { processor_.selectFactoryBank(bankIndex); });
        }
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
        processor_.selectProgramFromUi((processor_.getCurrentProgram() + 31) % 32);
        refresh(false);
    };
    next_.onClick = [this]
    {
        processor_.selectProgramFromUi((processor_.getCurrentProgram() + 1) % 32);
        refresh(false);
    };

    processor_.synchroniseOperatorParametersFromEngine();
    lastOperatorVoiceRevision_ = processor_.getOperatorVoiceRevision();
    selectOperator(0);
    updateVoiceValueLabels();
    masterVolume_.onValueChange();
    updateResponsiveTypography();
    startTimerHz(30);
    refresh(true);
}

VDX7AudioProcessorEditor::~VDX7AudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void VDX7AudioProcessorEditor::configureLabel(juce::Label& label, float fontSize,
                                               juce::Justification justification,
                                               juce::Colour colour)
{
    label.setFont(juce::Font(juce::FontOptions(fontSize)));
    label.setJustificationType(justification);
    label.setColour(juce::Label::textColourId, colour);
    label.setMinimumHorizontalScale(0.72f);
    label.setInterceptsMouseClicks(false, false);
}

void VDX7AudioProcessorEditor::configureTab(juce::TextButton& button, int radioGroup)
{
    button.getProperties().set("vdx7Tab", true);
    button.setClickingTogglesState(true);
    button.setRadioGroupId(radioGroup);
}

void VDX7AudioProcessorEditor::configureOperatorSlider(juce::Slider& slider, bool isFader)
{
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setSliderSnapsToMousePosition(false);
    slider.setScrollWheelEnabled(true);

    if (isFader)
    {
        slider.setSliderStyle(juce::Slider::LinearVertical);
        slider.getProperties().set("vdx7Fader", true);
        slider.setMouseDragSensitivity(170);
    }
    else
    {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setRotaryParameters(juce::MathConstants<float>::pi * 1.25f,
                                   juce::MathConstants<float>::pi * 2.75f, true);
        slider.setMouseDragSensitivity(180);
    }
}

void VDX7AudioProcessorEditor::selectOperator(int operatorIndex)
{
    selectedOperator_ = juce::jlimit(0, VDX7VoiceData::kOperatorCount - 1,
                                     operatorIndex);
    for (int op = 0; op < VDX7VoiceData::kOperatorCount; ++op)
        operatorTabs_[static_cast<std::size_t>(op)].setToggleState(
            op == selectedOperator_, juce::dontSendNotification);

    operatorTitle_.setText("OPERATOR " + juce::String(selectedOperator_ + 1),
                           juce::dontSendNotification);
    bindSelectedOperatorParameters();
    updateOperatorValueLabels();
    repaint(referenceRect(34, 526, 1372, 258));
    algorithmView_.setState(juce::roundToInt(voiceKnobs_[0].getValue()), selectedOperator_,
                            juce::roundToInt(voiceKnobs_[1].getValue()));
}

void VDX7AudioProcessorEditor::bindSelectedOperatorParameters()
{
    for (auto& attachment : operatorKnobAttachments_)
        attachment.reset();
    for (auto& attachment : envelopeAttachments_)
        attachment.reset();
    for (auto& attachment : operatorScaleAttachments_)
        attachment.reset();

    auto& parameters = processor_.parameters();
    for (std::size_t i = 0; i < operatorKnobAttachments_.size(); ++i)
        operatorKnobAttachments_[i] = std::make_unique<SliderAttachment>(
            parameters, VDX7ParameterIDs::operatorParameter(selectedOperator_, kKnobParameters[i]),
            operatorKnobs_[i]);

    for (std::size_t i = 0; i < envelopeAttachments_.size(); ++i)
        envelopeAttachments_[i] = std::make_unique<SliderAttachment>(
            parameters, VDX7ParameterIDs::operatorParameter(selectedOperator_, kEnvelopeParameters[i]),
            envelopeFaders_[i]);

    for (std::size_t i = 0; i < operatorScaleAttachments_.size(); ++i)
        operatorScaleAttachments_[i] = std::make_unique<SliderAttachment>(
            parameters, VDX7ParameterIDs::operatorParameter(selectedOperator_, kScaleParameters[i]),
            operatorScaleKnobs_[i]);
}

void VDX7AudioProcessorEditor::updateOperatorValueLabels()
{
    for (std::size_t i = 0; i < operatorKnobValues_.size(); ++i)
    {
        const int value = juce::roundToInt(operatorKnobs_[i].getValue());
        const auto text = kKnobParameters[i] == VDX7VoiceData::Parameter::detune && value > 0
            ? "+" + juce::String(value) : juce::String(value);
        operatorKnobValues_[i].setText(text, juce::dontSendNotification);
    }

    for (std::size_t i = 0; i < envelopeValues_.size(); ++i)
        envelopeValues_[i].setText(
            juce::String(juce::roundToInt(envelopeFaders_[i].getValue())),
            juce::dontSendNotification);

    for (std::size_t i = 0; i < operatorScaleValues_.size(); ++i)
    {
        const int value = juce::roundToInt(operatorScaleKnobs_[i].getValue());
        juce::String text = juce::String(value);
        if (kScaleParameters[i] == VDX7VoiceData::Parameter::oscillatorMode)
            text = value == 0 ? "RATIO" : "FIXED";
        else if (kScaleParameters[i] == VDX7VoiceData::Parameter::breakpoint)
            text = breakpointName(value);
        else if (kScaleParameters[i] == VDX7VoiceData::Parameter::leftScaleCurve
                 || kScaleParameters[i] == VDX7VoiceData::Parameter::rightScaleCurve)
            text = curveName(value);
        operatorScaleValues_[i].setText(text, juce::dontSendNotification);
    }

    const int mode = juce::roundToInt(operatorScaleKnobs_[0].getValue());
    const int coarse = juce::roundToInt(operatorKnobs_[1].getValue());
    const int fine = juce::roundToInt(operatorKnobs_[2].getValue());
    if (mode == 0)
    {
        const double ratio = (coarse == 0 ? 0.5 : static_cast<double>(coarse))
                           * (1.0 + static_cast<double>(fine) / 100.0);
        frequencyValue_.setText("RATIO  " + juce::String(ratio, 2) + " x",
                                juce::dontSendNotification);
    }
    else
    {
        const double hertz = std::pow(10.0, static_cast<double>(coarse % 4)
                                           + static_cast<double>(fine) / 100.0);
        frequencyValue_.setText("FIXED  " + juce::String(hertz, hertz < 1000.0 ? 1 : 0) + " Hz",
                                juce::dontSendNotification);
    }
}

void VDX7AudioProcessorEditor::updateVoiceValueLabels()
{
    for (std::size_t i = 0; i < pitchEnvelopeValues_.size(); ++i)
        pitchEnvelopeValues_[i].setText(
            juce::String(juce::roundToInt(pitchEnvelopeFaders_[i].getValue())),
            juce::dontSendNotification);

    for (std::size_t i = 0; i < voiceKnobValues_.size(); ++i)
    {
        const auto parameter = kVoiceKnobParameters[i];
        const int value = juce::roundToInt(voiceKnobs_[i].getValue());
        juce::String text = juce::String(value);
        if (parameter == VDX7VoiceData::VoiceParameter::oscillatorKeySync
            || parameter == VDX7VoiceData::VoiceParameter::lfoKeySync)
            text = value == 0 ? "OFF" : "ON";
        else if (parameter == VDX7VoiceData::VoiceParameter::lfoWaveform)
            text = waveformName(value);
        else if (parameter == VDX7VoiceData::VoiceParameter::transpose)
            text = (value > 0 ? "+" : "") + juce::String(value) + " st";
        voiceKnobValues_[i].setText(text, juce::dontSendNotification);
    }

    algorithmDisplay_.setText(
        juce::String(juce::roundToInt(voiceKnobs_[0].getValue())),
        juce::dontSendNotification);
    algorithmView_.setState(juce::roundToInt(voiceKnobs_[0].getValue()), selectedOperator_,
                            juce::roundToInt(voiceKnobs_[1].getValue()));
}

void VDX7AudioProcessorEditor::drawOperatorEnvelope(juce::Graphics& g)
{
    const float scaleY = static_cast<float>(getHeight()) / kReferenceHeight;
    const auto graph = referenceRect(1084, 580, 300, 164).toFloat();
    drawEnvelopeGrid(g, graph);
    g.setColour(juce::Colour(0xff17343a));
    g.drawRoundedRectangle(graph, 4.0f * scaleY, juce::jmax(1.0f, scaleY));

    std::array<float, 4> rates {};
    std::array<float, 4> levels {};
    float durationTotal = 0.0f;
    for (std::size_t stage = 0; stage < 4; ++stage)
    {
        rates[stage] = static_cast<float>(envelopeFaders_[stage].getValue());
        levels[stage] = static_cast<float>(envelopeFaders_[stage + 4].getValue());
        durationTotal += juce::jmax(0.08f, (100.0f - rates[stage]) / 100.0f);
    }

    const auto inner = graph.reduced(10.0f * scaleY, 9.0f * scaleY);
    const auto levelY = [&inner](float level)
    {
        return juce::jmap(juce::jlimit(0.0f, 99.0f, level), 0.0f, 99.0f,
                          inner.getBottom(), inner.getY());
    };

    juce::Path envelope;
    float x = inner.getX();
    envelope.startNewSubPath(x, levelY(levels[3]));
    float releaseX = x;
    std::array<juce::Point<float>, 4> points {};
    for (std::size_t stage = 0; stage < 4; ++stage)
    {
        const float duration = juce::jmax(0.08f, (100.0f - rates[stage]) / 100.0f);
        x += inner.getWidth() * duration / durationTotal;
        points[stage] = { x, levelY(levels[stage]) };
        envelope.lineTo(points[stage]);
        if (stage == 2)
            releaseX = x;
    }

    g.setColour(juce::Colour(0x5514b7bd));
    const float dashLengths[] { 3.0f * scaleY, 3.0f * scaleY };
    juce::Path releaseMarker;
    releaseMarker.startNewSubPath(releaseX, inner.getY());
    releaseMarker.lineTo(releaseX, inner.getBottom());
    juce::Path dashedReleaseMarker;
    juce::PathStrokeType(1.0f * scaleY).createDashedStroke(
        dashedReleaseMarker, releaseMarker, dashLengths, 2);
    g.fillPath(dashedReleaseMarker);

    g.setColour(juce::Colour(0xff00e7e7));
    g.strokePath(envelope, juce::PathStrokeType(2.0f * scaleY,
                                                juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));
    for (const auto point : points)
        g.fillEllipse(juce::Rectangle<float>(5.0f * scaleY, 5.0f * scaleY)
                          .withCentre(point));
}

void VDX7AudioProcessorEditor::drawPitchEnvelope(juce::Graphics& g)
{
    const float scaleY = static_cast<float>(getHeight()) / kReferenceHeight;
    const auto graph = referenceRect(326, 389, 264, 112).toFloat();
    drawEnvelopeGrid(g, graph);
    g.setColour(juce::Colour(0xff17343a));
    g.drawRoundedRectangle(graph, 3.0f * scaleY, juce::jmax(1.0f, scaleY));

    std::array<float, 4> rates {};
    std::array<float, 4> levels {};
    float durationTotal = 0.0f;
    for (std::size_t stage = 0; stage < 4; ++stage)
    {
        rates[stage] = static_cast<float>(pitchEnvelopeFaders_[stage].getValue());
        levels[stage] = static_cast<float>(pitchEnvelopeFaders_[stage + 4].getValue());
        durationTotal += juce::jmax(0.08f, (100.0f - rates[stage]) / 100.0f);
    }

    const auto inner = graph.reduced(8.0f * scaleY, 7.0f * scaleY);
    const auto levelY = [&inner](float level)
    {
        return juce::jmap(juce::jlimit(0.0f, 99.0f, level), 0.0f, 99.0f,
                          inner.getBottom(), inner.getY());
    };

    juce::Path envelope;
    float x = inner.getX();
    envelope.startNewSubPath(x, levelY(levels[3]));
    float releaseX = x;
    for (std::size_t stage = 0; stage < 4; ++stage)
    {
        const float duration = juce::jmax(0.08f, (100.0f - rates[stage]) / 100.0f);
        x += inner.getWidth() * duration / durationTotal;
        envelope.lineTo(x, levelY(levels[stage]));
        if (stage == 2)
            releaseX = x;
    }

    g.setColour(juce::Colour(0x5514b7bd));
    const float dashLengths[] { 3.0f * scaleY, 3.0f * scaleY };
    juce::Path releaseMarker;
    releaseMarker.startNewSubPath(releaseX, inner.getY());
    releaseMarker.lineTo(releaseX, inner.getBottom());
    juce::Path dashedReleaseMarker;
    juce::PathStrokeType(1.0f * scaleY).createDashedStroke(
        dashedReleaseMarker, releaseMarker, dashLengths, 2);
    g.fillPath(dashedReleaseMarker);

    g.setColour(juce::Colour(0x6614b7bd));
    g.drawHorizontalLine(juce::roundToInt(levelY(50.0f)), inner.getX(), inner.getRight());
    g.setColour(juce::Colour(0xff00e7e7));
    g.strokePath(envelope, juce::PathStrokeType(1.8f * scaleY,
                                                juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));
}

void VDX7AudioProcessorEditor::updateResponsiveTypography()
{
    const float scale = juce::jmin(static_cast<float>(getWidth()) / kReferenceWidth,
                                   static_cast<float>(getHeight()) / kReferenceHeight);
    const auto setFont = [scale](juce::Label& label, float referenceSize, int style = juce::Font::plain)
    {
        label.setFont(juce::Font(juce::FontOptions(
            juce::jmax(8.0f, referenceSize * scale), style)));
    };

    setFont(mkLabel_, 37.0f);
    setFont(hardwareLabel_, 14.0f, juce::Font::bold);
    setFont(firmwareLabel_, 14.0f);
    setFont(headerPatch_, 18.0f, juce::Font::bold);
    setFont(status_, 12.0f);
    setFont(patch_, 29.0f, juce::Font::bold);
    setFont(bankCaption_, 14.0f);
    setFont(programCaption_, 14.0f);
    setFont(masterCaption_, 14.0f);
    setFont(masterValue_, 17.0f);
    setFont(pitchCaption_, 14.0f);
    setFont(modCaption_, 14.0f);
    setFont(outputCaption_, 19.0f);
    setFont(leftCaption_, 12.0f);
    setFont(rightCaption_, 12.0f);
    setFont(algorithmDisplay_, 18.0f, juce::Font::bold);
    setFont(operatorTitle_, 20.0f);
    setFont(frequencyValue_, 10.0f);
    setFont(pitchEnvelopeTitle_, 12.0f, juce::Font::bold);
    setFont(voiceLfoTitle_, 12.0f, juce::Font::bold);
    setFont(envelopeTitle_, 13.0f, juce::Font::bold);
    for (auto& label : operatorScaleCaptions_) setFont(label, 10.0f);
    for (auto& label : operatorScaleValues_) setFont(label, 11.0f);
    for (auto& label : pitchEnvelopeCaptions_) setFont(label, 10.0f);
    for (auto& label : pitchEnvelopeValues_) setFont(label, 10.0f);
    for (auto& label : voiceKnobCaptions_) setFont(label, 9.0f);
    for (auto& label : voiceKnobValues_) setFont(label, 10.0f);
    for (auto& label : operatorKnobCaptions_) setFont(label, 11.0f, juce::Font::bold);
    for (auto& label : operatorKnobValues_) setFont(label, 12.0f);
    for (auto& label : envelopeCaptions_) setFont(label, 11.0f, juce::Font::bold);
    for (auto& label : envelopeValues_) setFont(label, 11.0f);
    setFont(footerLeft_, 15.0f);
    setFont(footerCentre_, 15.0f);
    setFont(footerRight_, 12.0f);
}

juce::Rectangle<int> VDX7AudioProcessorEditor::referenceRect(float x, float y,
                                                              float width, float height) const
{
    const float scaleX = static_cast<float>(getWidth()) / kReferenceWidth;
    const float scaleY = static_cast<float>(getHeight()) / kReferenceHeight;
    return juce::Rectangle<float>(x * scaleX, y * scaleY, width * scaleX, height * scaleY)
        .getSmallestIntegerContainer();
}

void VDX7AudioProcessorEditor::paint(juce::Graphics& g)
{
    const float scaleY = static_cast<float>(getHeight()) / kReferenceHeight;
    g.fillAll(juce::Colour(0xff05090b));

    // The source chassis PNG deliberately contains 50 transparent pixels above
    // its top edge. Compress only that upper chrome into a smaller 18 px inset,
    // then keep the complete GUI area below y=135 pixel-aligned with the concept.
    // This lifts the frame without moving any controls, panels or the footer.
    const auto upperChassis = referenceRect(0, 18, 1440, 117).toFloat();
    g.drawImage(chassis_, upperChassis.getX(), upperChassis.getY(),
                upperChassis.getWidth(), upperChassis.getHeight(),
                0, 50, 1440, 85);
    // Extend background chrome only: controls retain their reference coordinates.
    const auto lowerChassis = referenceRect(0, 135, 1440, 975).toFloat();
    g.drawImage(chassis_, lowerChassis.getX(), lowerChassis.getY(),
                lowerChassis.getWidth(), lowerChassis.getHeight(),
                0, 135, 1440, 945);

    const auto drawPanel = [this, &g](float x, float y, float width, float height)
    {
        g.drawImage(panel_, referenceRect(x, y, width, height).toFloat());
    };

    drawPanel(34, 135, 920, 190);
    drawPanel(968, 135, 220, 190);
    drawPanel(1200, 125, 206, 401);
    drawPanel(34, 340, 1154, 175);
    drawPanel(34, 526, 1372, 270);
    drawPanel(34, 798, 1372, 180);

    g.drawImage(wordmark_, referenceRect(44, 58, 300, 72).toFloat());
    g.drawImage(lcdFrame_, referenceRect(392, 150, 543, 166).toFloat());
    const auto footerBounds = referenceRect(18, 990, 1404, 44).toFloat();
    juce::ColourGradient footerGradient(juce::Colour(0xff071014), footerBounds.getX(),
                                        footerBounds.getCentreY(), juce::Colour(0xff071014),
                                        footerBounds.getRight(), footerBounds.getCentreY(), false);
    footerGradient.addColour(0.5, juce::Colour(0xff0b181b));
    g.setGradientFill(footerGradient);
    g.fillRoundedRectangle(footerBounds, 3.0f * scaleY);
    g.setColour(juce::Colour(0xff25454c));
    g.fillRect(footerBounds.withHeight(juce::jmax(1.0f, scaleY)));
    g.drawImage(valueField_, referenceRect(1255, 467, 96, 32).toFloat());

    g.setFont(juce::Font(juce::FontOptions(20.0f * scaleY)));
    g.setColour(juce::Colour(0xffe6eef0));
    g.drawText("VOICE", referenceRect(52, 145, 250, 30), juce::Justification::centredLeft);
    g.drawText("GLOBAL", referenceRect(52, 350, 250, 30), juce::Justification::centredLeft);
    g.drawText("ALGORITHM", referenceRect(986, 145, 155, 30), juce::Justification::centredLeft);

    g.drawImage(divider_, referenceRect(52, 178, 280, 2).toFloat());
    g.drawImage(divider_, referenceRect(52, 382, 1115, 2).toFloat());
    g.drawImage(divider_, referenceRect(52, 568, 1110, 2).toFloat());

    g.setFont(juce::Font(juce::FontOptions(11.0f * scaleY)));
    g.setColour(juce::Colour(0xff91a5ac));

    g.setColour(juce::Colour(0xff24444b));
    g.fillRect(referenceRect(594, 390, 1, 116));
    drawPitchEnvelope(g);
    drawOperatorEnvelope(g);
}

void VDX7AudioProcessorEditor::resized()
{
    updateResponsiveTypography();

    mkLabel_.setBounds(referenceRect(360, 88, 85, 44));
    hardwareLabel_.setBounds(referenceRect(450, 94, 220, 17));
    firmwareLabel_.setBounds(referenceRect(450, 108, 220, 19));
    previous_.setBounds(referenceRect(652, 76, 34, 46));
    headerPatch_.setBounds(referenceRect(686, 76, 208, 46));
    next_.setBounds(referenceRect(894, 76, 34, 46));
    loadRom_.setBounds(referenceRect(1000, 76, 104, 46));
    loadSyx_.setBounds(referenceRect(1106, 76, 104, 46));
    settings_.setBounds(referenceRect(1212, 76, 104, 46));
    about_.setBounds(referenceRect(1318, 76, 90, 46));
    for (auto* button : { &loadRom_, &loadSyx_, &settings_, &about_ })
        button->getProperties().set("vdx7WideHeader", true);
    status_.setBounds(referenceRect(56, 242, 324, 64));

    editTab_.setBounds(referenceRect(54, 190, 100, 42));
    performanceTab_.setBounds(referenceRect(160, 190, 118, 42));
    utilityTab_.setBounds(referenceRect(284, 190, 100, 42));
    patch_.setBounds(referenceRect(421, 190, 473, 40));
    bankCaption_.setBounds(referenceRect(421, 238, 52, 24));
    bank_.setBounds(referenceRect(473, 238, 94, 24));
    programCaption_.setBounds(referenceRect(595, 238, 76, 24));
    program_.setBounds(referenceRect(671, 238, 72, 24));

    outputCaption_.setBounds(referenceRect(1210, 146, 186, 30));
    leftCaption_.setBounds(referenceRect(1210, 178, 58, 22));
    masterCaption_.setBounds(referenceRect(1270, 178, 66, 22));
    rightCaption_.setBounds(referenceRect(1338, 178, 58, 22));
    leftMeter_.setBounds(referenceRect(1220, 202, 38, 232));
    masterVolume_.setBounds(referenceRect(1274, 202, 58, 232));
    rightMeter_.setBounds(referenceRect(1348, 202, 38, 232));
    masterValue_.setBounds(referenceRect(1255, 467, 96, 32));
    algorithmDisplay_.setBounds(referenceRect(1143, 145, 34, 30));
    algorithmView_.setBounds(referenceRect(980, 180, 200, 124));
    pitchEnvelopeTitle_.setBounds(referenceRect(52, 384, 260, 20));
    voiceLfoTitle_.setBounds(referenceRect(602, 384, 260, 20));
    for (std::size_t i = 0; i < pitchEnvelopeFaders_.size(); ++i)
    {
        const float x = 52.0f + static_cast<float>(i) * 33.0f;
        pitchEnvelopeCaptions_[i].setBounds(referenceRect(x, 404, 28, 16));
        pitchEnvelopeFaders_[i].setBounds(referenceRect(x, 422, 28, 62));
        pitchEnvelopeValues_[i].setBounds(referenceRect(x, 486, 28, 18));
    }
    for (std::size_t i = 0; i < voiceKnobs_.size(); ++i)
    {
        const float x = 602.0f + static_cast<float>(i) * 51.0f;
        voiceKnobCaptions_[i].setBounds(referenceRect(x, 404, 48, 16));
        voiceKnobs_[i].setBounds(referenceRect(x, 426, 48, 48));
        voiceKnobValues_[i].setBounds(referenceRect(x, 486, 48, 18));
    }

    for (int op = 0; op < VDX7VoiceData::kOperatorCount; ++op)
        operatorTabs_[static_cast<std::size_t>(op)].setBounds(
            referenceRect(42.0f + op * 104.0f, 532, 96, 38));

    operatorTitle_.setBounds(referenceRect(52, 578, 260, 28));
    frequencyValue_.setBounds(referenceRect(52, 751, 92, 16));
    envelopeTitle_.setBounds(referenceRect(704, 578, 240, 28));
    for (std::size_t i = 0; i < operatorKnobs_.size(); ++i)
    {
        const float x = 48.0f + static_cast<float>(i) * 92.0f;
        operatorKnobCaptions_[i].setBounds(referenceRect(x, 606, 80, 18));
        operatorKnobs_[i].setBounds(referenceRect(x + 14.0f, 624, 52, 52));
        operatorKnobValues_[i].setBounds(referenceRect(x, 677, 80, 20));
    }
    for (std::size_t i = 0; i < operatorScaleKnobs_.size(); ++i)
    {
        const float x = 48.0f + static_cast<float>(i) * 106.0f;
        operatorScaleCaptions_[i].setBounds(referenceRect(x, 697, 92, 16));
        operatorScaleKnobs_[i].setBounds(referenceRect(x + 27.0f, 713, 38, 38));
        if (i == 0)
            operatorScaleKnobs_[i].setBounds(referenceRect(x + 17.0f, 713, 58, 38));
        operatorScaleValues_[i].setBounds(referenceRect(x, 751, 92, 16));
    }

    for (std::size_t i = 0; i < envelopeFaders_.size(); ++i)
    {
        const float x = 704.0f + static_cast<float>(i) * 47.0f;
        envelopeCaptions_[i].setBounds(referenceRect(x, 606, 36, 18));
        envelopeFaders_[i].setBounds(referenceRect(x, 626, 36, 116));
        envelopeValues_[i].setBounds(referenceRect(x, 746, 36, 20));
    }

    pitchCaption_.setBounds(referenceRect(48, 807, 72, 24));
    modCaption_.setBounds(referenceRect(124, 807, 72, 24));
    pitchWheel_.setBounds(referenceRect(64.75f, 830, 40.5f, 138));
    modWheel_.setBounds(referenceRect(140.75f, 830, 40.5f, 138));
    keyboard_.setBounds(referenceRect(210, 830, 1184, 138));
    keyboard_.setKeyWidth(static_cast<float>(keyboard_.getWidth()) / 36.0f);
    keyboard_.setLowestVisibleKey(36);

    footerLeft_.setBounds(referenceRect(38, 994, 300, 34));
    footerCentre_.setBounds(referenceRect(480, 994, 480, 34));
    footerRight_.setBounds(referenceRect(1050, 994, 350, 34));
}

void VDX7AudioProcessorEditor::timerCallback()
{
    const float left = processor_.getOutputPeak(0);
    const float right = processor_.getOutputPeak(1);
    displayedLeftPeak_ = left > displayedLeftPeak_ ? left : displayedLeftPeak_ * 0.88f;
    displayedRightPeak_ = right > displayedRightPeak_ ? right : displayedRightPeak_ * 0.88f;
    leftMeter_.setLevel(displayedLeftPeak_);
    rightMeter_.setLevel(displayedRightPeak_);

    const double rawCpuPercent = juce::jlimit(0.0, 100.0, processor_.getCpuUsagePercent());
    if (!cpuDisplayInitialised_)
    {
        displayedCpuPercent_ = rawCpuPercent;
        cpuDisplayInitialised_ = true;
    }
    else
    {
        displayedCpuPercent_ += kCpuDisplaySmoothing
                              * (rawCpuPercent - displayedCpuPercent_);
    }

    if (++cpuTextRefreshCounter_ >= kCpuTextRefreshFrames)
    {
        cpuTextRefreshCounter_ = 0;
        const double stableCpuPercent = std::round(displayedCpuPercent_ * 2.0) * 0.5;
        footerRight_.setText("CPU " + juce::String(stableCpuPercent, 1) + "%",
                             juce::dontSendNotification);
    }

    const auto operatorRevision = processor_.getOperatorVoiceRevision();
    if (operatorRevision != lastOperatorVoiceRevision_)
    {
        lastOperatorVoiceRevision_ = operatorRevision;
        updateOperatorValueLabels();
        updateVoiceValueLabels();
        repaint();
    }

    ++metadataRefreshCounter_;
    refresh(metadataRefreshCounter_ >= 6);
    if (metadataRefreshCounter_ >= 6)
        metadataRefreshCounter_ = 0;
}

void VDX7AudioProcessorEditor::refresh(bool refreshMetadata)
{
    internalUiUpdate_ = true;

    const bool loaded = processor_.isRomLoaded();
    const bool factories = processor_.hasFactoryVoices();
    const auto patchName = loaded ? processor_.getCurrentPatchName()
        + (processor_.isCurrentVoiceModified() ? " *" : "") : juce::String("LOAD DX7 ROM");
    const int bankIndex = processor_.getCurrentBank();
    const int programIndex = processor_.getCurrentProgram();

    headerPatch_.setText(patchName, juce::dontSendNotification);
    patch_.setText(juce::String(programIndex + 1).paddedLeft('0', 2) + "   " + patchName,
                   juce::dontSendNotification);

    bank_.setSelectedId(bankIndex >= 0 ? bankIndex + 1 : 0, juce::dontSendNotification);
    bank_.setTextWhenNothingSelected(loaded ? "CUSTOM" : "");
    program_.setSelectedId(programIndex + 1, juce::dontSendNotification);
    bank_.setEnabled(loaded && factories);
    program_.setEnabled(loaded);
    loadSyx_.setEnabled(loaded);
    utilityTab_.setEnabled(loaded);
    algorithmView_.setEnabled(loaded);
    previous_.setEnabled(loaded);
    next_.setEnabled(loaded);
    keyboard_.setEnabled(loaded);
    for (auto& tab : operatorTabs_) tab.setEnabled(loaded);
    for (auto& slider : operatorKnobs_) slider.setEnabled(loaded);
    for (auto& slider : envelopeFaders_) slider.setEnabled(loaded);
    for (auto& slider : operatorScaleKnobs_) slider.setEnabled(loaded);
    for (auto& slider : pitchEnvelopeFaders_) slider.setEnabled(loaded);
    for (auto& slider : voiceKnobs_) slider.setEnabled(loaded);

    if (refreshMetadata)
    {
        status_.setText(processor_.hasUnexportedEdits() ? "Bank has unexported edits\nUTILITY to save"
                        : processor_.getStatusText(), juce::dontSendNotification);
        loadRom_.setTooltip(processor_.getRomPath());
    }

    internalUiUpdate_ = false;
}

void VDX7AudioProcessorEditor::chooseRom()
{
    chooser_ = std::make_unique<juce::FileChooser>(
        "Select DX7 Mk I firmware",
        processor_.getSuggestedRomFolder(),
        "*.bin;*.BIN;*.obj;*.OBJ");

    chooser_->launchAsync(juce::FileBrowserComponent::openMode
                              | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& chooser)
        {
            const auto file = chooser.getResult();
            if (!file.existsAsFile()) return;
            confirmReplacement([this, file]
            {
                juce::String error;
                if (!processor_.loadRomFromFile(file, &error))
                    showError("VDX7 ROM error", error);
                refresh(true);
            });
        });
}

void VDX7AudioProcessorEditor::chooseSyx()
{
    chooser_ = std::make_unique<juce::FileChooser>(
        "Select a DX7 voice or 32-voice SysEx bank",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
        "*.syx;*.SYX");

    chooser_->launchAsync(juce::FileBrowserComponent::openMode
                              | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& chooser)
        {
            const auto file = chooser.getResult();
            if (!file.existsAsFile()) return;
            confirmReplacement([this, file]
            {
                juce::String error;
                if (!processor_.loadSyxFromFile(file, &error))
                    showError("VDX7 SysEx error", error);
                refresh(true);
            });
        });
}

void VDX7AudioProcessorEditor::confirmReplacement(std::function<void()> action)
{
    if (!processor_.hasUnexportedEdits()) { action(); return; }
    juce::Component::SafePointer<VDX7AudioProcessorEditor> safe(this);
    juce::AlertWindow::showOkCancelBox(juce::MessageBoxIconType::WarningIcon,
        "Unexported voice edits",
        "This action can replace edited sounds. Cancel and use UTILITY > Save bank first "
        "to preserve all edited voices in a separate file. Your DAW project saves remain independent.",
        "Continue", "Cancel", nullptr,
        juce::ModalCallbackFunction::create([safe, action = std::move(action)](int result)
        {
            if (safe == nullptr) return;
            if (result != 0) action();
            safe->refresh(true);
        }));
}

void VDX7AudioProcessorEditor::showUtilityMenu()
{
    juce::PopupMenu menu;
    menu.addItem(1, "Rename voice...");
    menu.addItem(2, "Save voice (.syx)...");
    menu.addItem(3, "Save bank - 32 voices (.syx)...");
    menu.addSeparator();
    menu.addItem(4, "Copy OP" + juce::String(selectedOperator_+1));
    menu.addItem(5, "Paste into OP" + juce::String(selectedOperator_+1), processor_.hasCopiedOperator());
    juce::Component::SafePointer<VDX7AudioProcessorEditor> safe(this);
    const int op = selectedOperator_;
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&utilityTab_),
        [safe, op](int result)
        {
            if (safe == nullptr) return;
            switch (result)
            {
                case 1: safe->renameVoice(); break;
                case 2: safe->chooseExport(false); break;
                case 3: safe->chooseExport(true); break;
                case 4: safe->processor_.copyOperator(op); break;
                case 5: safe->processor_.pasteOperator(op); break;
                default: break;
            }
            safe->refresh(true);
        });
}

void VDX7AudioProcessorEditor::renameVoice()
{
    auto* dialog = new juce::AlertWindow("Rename voice", "Use 1-10 printable ASCII characters.",
                                        juce::MessageBoxIconType::NoIcon);
    dialog->addTextEditor("name", processor_.getCurrentPatchName(), "Name:");
    dialog->getTextEditor("name")->setInputRestrictions(10);
    dialog->addButton("Rename", 1, juce::KeyPress(juce::KeyPress::returnKey));
    dialog->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
    juce::Component::SafePointer<VDX7AudioProcessorEditor> safe(this);
    const int program = processor_.getCurrentProgram();
    const auto revision = processor_.getOperatorVoiceRevision();
    dialog->enterModalState(true, juce::ModalCallbackFunction::create(
        [safe, dialog, program, revision](int result)
        {
            if (safe == nullptr || result != 1) return;
            if (safe->processor_.getCurrentProgram() != program
                || safe->processor_.getOperatorVoiceRevision() != revision)
                safe->showError("Voice changed", "The voice changed while the name dialog was open. Please try again.");
            else if (!safe->processor_.renameVoice(dialog->getTextEditorContents("name")))
                safe->showError("Invalid name", "Use 1-10 printable ASCII characters (no accents).");
            safe->refresh(true);
        }), true);
}

void VDX7AudioProcessorEditor::chooseExport(bool entireBank)
{
    const int program = processor_.getCurrentProgram();
    const auto revision = processor_.getOperatorVoiceRevision();
    chooser_ = std::make_unique<juce::FileChooser>(
        entireBank ? "Save current 32-voice bank" : "Save current voice",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile(
            entireBank ? "VDX7-bank.syx" : juce::File::createLegalFileName(processor_.getCurrentPatchName()) + ".syx"),
        "*.syx");
    chooser_->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                          | juce::FileBrowserComponent::warnAboutOverwriting,
        [this, entireBank, program, revision](const juce::FileChooser& chooser)
        {
            const auto file = chooser.getResult();
            if (file == juce::File()) return;
            if (processor_.getCurrentProgram() != program || processor_.getOperatorVoiceRevision() != revision)
            {
                showError("Voice changed", "The voice/bank changed while the save dialog was open. Please try again.");
                return;
            }
            juce::String error;
            if (!processor_.exportSyx(file, entireBank, error)) showError("Save failed", error);
            refresh(true);
        });
}

void VDX7AudioProcessorEditor::showError(const juce::String& title, const juce::String& message)
{
    juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, title, message);
}
