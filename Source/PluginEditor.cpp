#include "PluginEditor.h"
#include "VDX7AboutPanel.h"
#include "VDX7StatusPresentation.h"

#include <BinaryData.h>

#include <array>
#include <cmath>

namespace
{
class SettingsLookAndFeel final : public VDX7LookAndFeel
{
public:
    juce::Font getAlertWindowMessageFont() override
    {
        return juce::Font(juce::FontOptions(12.5f));
    }
};

class SettingsInfoComponent final : public juce::Component
{
public:
    explicit SettingsInfoComponent(juce::StringArray rows) : rows_(std::move(rows))
    {
        const juce::Font infoFont(juce::FontOptions(12.5f));
        int textWidth = 0;
        for (const auto& row : rows_)
            textWidth = juce::jmax(textWidth, juce::roundToInt(
                juce::GlyphArrangement::getStringWidth(infoFont, row)));

        // AlertWindow sizes its controls to 80% of the dialog width and sizes
        // the dialog from this custom component. Keep both the information
        // rows and the controls fitted to the longest information line.
        setSize(textWidth + 24, rows_.size() * rowHeight);
        setInterceptsMouseClicks(false, false);
    }

    void paint(juce::Graphics& g) override
    {
        g.setColour(juce::Colour(0xffeee9dc));
        g.setFont(juce::Font(juce::FontOptions(12.5f)));
        for (int i = 0; i < rows_.size(); ++i)
            g.drawText(rows_[i], 0, i * rowHeight, getWidth(), rowHeight,
                       juce::Justification::centred, false);
    }

private:
    static constexpr int rowHeight = 24;
    juce::StringArray rows_;
};

constexpr float kReferenceWidth = 1440.0f;
constexpr float kReferenceHeight = 1110.0f;
constexpr int kTabGroup = 1001;
constexpr int kOperatorTabGroup = 1002;
constexpr double kCpuDisplaySmoothing = 0.015;
constexpr int kCpuTextRefreshFrames = 15;

void drawEnvelopeGrid(juce::Graphics& g, juce::Rectangle<float> bounds, float scale)
{
    const float bezel = 2.0f * scale;
    g.setColour(juce::Colour(0xff050605));
    g.fillRoundedRectangle(bounds, 3.0f * scale);
    const auto screen = bounds.reduced(bezel);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff101512), screen.getTopLeft(),
        juce::Colour(0xff222a22), screen.getBottomRight(), false));
    g.fillRoundedRectangle(screen, 2.0f * scale);
    g.setColour(juce::Colour(0xff080a08));
    g.drawRoundedRectangle(screen, 2.0f * scale, juce::jmax(0.6f, scale - 1.0f));
    // A restrained top-edge glint and lower shadow make the black rim read as
    // an inset display rather than a flat line painted over the panel.
    g.setColour(juce::Colour(0xff697267).withAlpha(0.32f));
    g.drawLine(screen.getX() + 3.0f * scale, screen.getY() + scale,
               screen.getRight() - 3.0f * scale, screen.getY() + scale, juce::jmax(0.7f, scale));
    g.setColour(juce::Colour(0xff050605).withAlpha(0.75f));
    g.drawLine(screen.getX() + 3.0f * scale, screen.getBottom() - scale,
               screen.getRight() - 3.0f * scale, screen.getBottom() - scale, juce::jmax(0.7f, scale));
    g.setColour(juce::Colour(0xff71685b));
    g.drawRoundedRectangle(bounds.expanded(scale), 3.0f * scale, scale);

    const auto grid = screen.reduced(1.0f * scale);
    g.setColour(juce::Colour(0xff40483a));
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

void drawChassisScrew(juce::Graphics& g, juce::Point<float> centre, float radius)
{
    g.setColour(juce::Colour(0xff111211).withAlpha(0.9f));
    g.fillEllipse(centre.x - radius, centre.y - radius + 1.0f, radius * 2.0f, radius * 2.0f);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff85847d), centre.x - radius,
        centre.y - radius, juce::Colour(0xff343632), centre.x + radius, centre.y + radius, false));
    g.fillEllipse(centre.x - radius + 1.0f, centre.y - radius + 1.0f,
                  (radius - 1.0f) * 2.0f, (radius - 1.0f) * 2.0f);
    g.setColour(juce::Colour(0xff171918));
    g.drawLine(centre.x - radius * 0.42f, centre.y - radius * 0.42f,
               centre.x + radius * 0.42f, centre.y + radius * 0.42f, juce::jmax(1.0f, radius * 0.22f));
    g.drawLine(centre.x + radius * 0.42f, centre.y - radius * 0.42f,
               centre.x - radius * 0.42f, centre.y + radius * 0.42f, juce::jmax(1.0f, radius * 0.22f));
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

constexpr std::array<VDX7VoiceData::VoiceParameter, 10> kVoiceKnobParameters
{
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

constexpr std::array<const char*, 10> kVoiceKnobCaptions
{
    "FDBK", "O-SYNC", "TRANS", "SPEED", "DELAY",
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
    setColour(whiteNoteColourId, juce::Colour(0xff24221f));
    setBlackNoteLengthProportion(84.0f / 132.0f);
    setBlackNoteWidthProportion(24.0f / 36.0f);
}

void VDX7Keyboard::paintOverChildren(juce::Graphics& g)
{
    // A stationary felt strip above the keys, including pressed/hovered notes.
    const float line = juce::jmax(1.0f, getHeight() / 138.0f);
    g.setColour(juce::Colour(0xff080909));
    g.fillRect(0.0f, 0.0f, float(getWidth()), line);
    g.setColour(juce::Colour(0xffc4454f));
    g.fillRect(0.0f, line, float(getWidth()), line);
    g.setColour(juce::Colour(0xff080909));
    g.fillRect(0.0f, line * 2.0f, float(getWidth()), line);
    g.setColour(juce::Colour(0xff8e1728));
    g.fillRect(0.0f, line * 3.0f, float(getWidth()), line);
}

void VDX7Keyboard::drawWhiteNote(int, juce::Graphics& g, juce::Rectangle<float> area,
                                  bool isDown, bool isOver, juce::Colour, juce::Colour)
{
    const auto keyBody = area.reduced(1.0f, area.getHeight() * 0.09f);
    g.setColour(juce::Colour(0xffeeeae2));
    g.fillRoundedRectangle(keyBody, 1.5f);
    g.drawImage(isDown ? whitePressed_ : whiteNormal_, area);
    if (isOver && !isDown)
    {
        g.setColour(juce::Colour(0x2600e7e7));
        g.fillRoundedRectangle(keyBody, 1.5f);
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
    constexpr int segmentCount = 24;
    const auto bounds = getLocalBounds().toFloat();
    const auto railBounds = bounds.withSizeKeepingCentre(bounds.getWidth() * 0.72f,
                                                          bounds.getHeight());
    g.setColour(juce::Colour(0xff151612));
    g.fillRoundedRectangle(railBounds, 3.0f);

    const float db = juce::Decibels::gainToDecibels(level_, -60.0f);
    const int litSegments = juce::jlimit(0, segmentCount,
        static_cast<int>(std::ceil((db + 60.0f) / 60.0f * segmentCount)));
    const float ledWidth = bounds.getWidth() * 0.42f;
    const float gap = juce::jmax(1.0f, bounds.getHeight() * 0.012f);
    const float ledHeight = (bounds.getHeight() - gap * (segmentCount + 1)) / segmentCount;

    for (int segment = 0; segment < segmentCount; ++segment)
    {
        const float y = bounds.getBottom() - gap - (segment + 1) * ledHeight - segment * gap;
        const auto ledBounds = juce::Rectangle<float>(ledWidth, ledHeight)
                                   .withCentre({ bounds.getCentreX(), y + ledHeight * 0.5f });

        const auto colour = segment >= 20 ? juce::Colour(0xffed5b48)
            : (segment >= 16 ? juce::Colour(0xffead55b) : juce::Colour(0xff64d653));
        if (segment < litSegments)
        {
            const auto glowBounds = ledBounds.expanded(ledWidth * 0.34f, ledHeight * 0.22f);
            juce::ColourGradient glow(colour.withAlpha(0.38f), ledBounds.getCentreX(), ledBounds.getCentreY(),
                                      colour.withAlpha(0.0f), glowBounds.getRight(), glowBounds.getBottom(), true);
            g.setGradientFill(glow);
            g.fillRoundedRectangle(glowBounds, 2.0f);

            juce::ColourGradient lens(colour.brighter(0.22f), ledBounds.getX(), ledBounds.getY(),
                                      colour.darker(0.16f), ledBounds.getX(), ledBounds.getBottom(), false);
            g.setGradientFill(lens);
            g.fillRoundedRectangle(ledBounds, 1.0f);
            g.setColour(juce::Colours::white.withAlpha(0.24f));
            g.fillRoundedRectangle(ledBounds.withHeight(juce::jmax(1.0f, ledHeight * 0.16f)), 0.8f);
        }
        else
        {
            const auto unlit = segment >= 20 ? juce::Colour(0xff100706)
                : (segment >= 16 ? juce::Colour(0xff100e05) : juce::Colour(0xff060d08));
            g.setColour(unlit);
            g.fillRoundedRectangle(ledBounds, 1.0f);
            g.setColour(unlit.brighter(0.07f));
            g.drawRoundedRectangle(ledBounds, 1.0f, 0.55f);
        }
    }
}

VDX7AudioProcessorEditor::VDX7AudioProcessorEditor(VDX7AudioProcessor& processor)
    : AudioProcessorEditor(&processor),
      processor_(processor),
      keyboard_(processor.keyboardState()),
      performancePanel_(processor),
      chassis_(loadImage(VDX7Assets::mainwindow_png, VDX7Assets::mainwindow_pngSize)),
      wordmark_(juce::Drawable::createFromImageData(VDX7Assets::vdx7silver2_svg, VDX7Assets::vdx7silver2_svgSize)),
      lcdFrame_(loadImage(VDX7Assets::lcdframe_png, VDX7Assets::lcdframe_pngSize)),
      panel_(loadImage(VDX7Assets::panel9slice_png, VDX7Assets::panel9slice_pngSize)),
      valueField_(loadImage(VDX7Assets::valuefield_png, VDX7Assets::valuefield_pngSize)),
      envelopeGrid_(loadImage(VDX7Assets::envelopegrid_png, VDX7Assets::envelopegrid_pngSize)),
      divider_(loadImage(VDX7Assets::sectiondivider_png, VDX7Assets::sectiondivider_pngSize))
{
    setLookAndFeel(&lookAndFeel_);
    setOpaque(true);
    setResizable(true, true);
    setResizeLimits(600, 463, 1800, 1388);
    if (auto* constrainer = getConstrainer())
        constrainer->setFixedAspectRatio(kReferenceWidth / kReferenceHeight);
    setSize(1200, 925);

    configureLabel(status_, 12.0f, juce::Justification::centredLeft, juce::Colour(0xffcec9bd));
    configureLabel(patch_, 29.0f, juce::Justification::centredLeft, juce::Colour(0xff273019));
    configureLabel(bankCaption_, 13.0f, juce::Justification::centredLeft, juce::Colour(0xff273019));
    configureLabel(programCaption_, 12.0f, juce::Justification::centredLeft, juce::Colour(0xff273019));
    configureLabel(masterCaption_, 14.0f, juce::Justification::centred);
    configureLabel(masterValue_, 17.0f, juce::Justification::centred, juce::Colour(0xff68c7bb));
    configureLabel(pitchCaption_, 15.5f, juce::Justification::centred);
    configureLabel(modCaption_, 14.0f, juce::Justification::centred);
    configureLabel(outputCaption_, 19.0f, juce::Justification::centred);
    configureLabel(leftCaption_, 12.0f, juce::Justification::centred);
    configureLabel(rightCaption_, 12.0f, juce::Justification::centred);
    configureLabel(operatorTitle_, 20.0f, juce::Justification::centredLeft);
    configureLabel(frequencyValue_, 14.0f, juce::Justification::centredRight,
                   juce::Colour(0xff68c7bb));
    configureLabel(envelopeTitle_, 13.0f, juce::Justification::centredLeft,
                   juce::Colour(0xffc9c4b8));
    configureLabel(pitchEnvelopeTitle_, 12.0f, juce::Justification::centredLeft,
                   juce::Colour(0xffc9c4b8));
    configureLabel(voiceLfoTitle_, 12.0f, juce::Justification::centredLeft,
                   juce::Colour(0xffc9c4b8));
    configureLabel(footerLeft_, 15.0f, juce::Justification::centredLeft);
    configureLabel(footerCentre_, 15.0f, juce::Justification::centred, juce::Colour(0xffb0aa9d));
    configureLabel(footerRight_, 12.0f, juce::Justification::centredRight, juce::Colour(0xffc9c4b8));

    masterCaption_.setText("VOLUME", juce::dontSendNotification);
    pitchCaption_.setText("PITCH", juce::dontSendNotification);
    modCaption_.setText("MOD", juce::dontSendNotification);
    outputCaption_.setText("OUTPUT", juce::dontSendNotification);
    leftCaption_.setText("LEFT", juce::dontSendNotification);
    rightCaption_.setText("RIGHT", juce::dontSendNotification);
    operatorTitle_.setText("OPERATOR 1", juce::dontSendNotification);
    frequencyValue_.setText("RATIO 1.00 x", juce::dontSendNotification);
    envelopeTitle_.setText("4-STAGE ENVELOPE", juce::dontSendNotification);
    pitchEnvelopeTitle_.setText("PITCH ENVELOPE", juce::dontSendNotification);
    voiceLfoTitle_.setText("VOICE + LFO", juce::dontSendNotification);
    footerLeft_.setText("VDX7 Mk I     v" VDX7_DISPLAY_VERSION, juce::dontSendNotification);
    footerCentre_.setText("6-OPERATOR FM SYNTHESIZER", juce::dontSendNotification);
    footerRight_.setText("CPU 0.0%", juce::dontSendNotification);

    for (auto* label : { &status_,
                         &patch_, &bankCaption_, &programCaption_, &masterCaption_,
                         &masterValue_, &pitchCaption_, &modCaption_, &outputCaption_, &leftCaption_,
                         &rightCaption_, &operatorTitle_, &frequencyValue_,
                         &envelopeTitle_, &pitchEnvelopeTitle_, &voiceLfoTitle_, &footerLeft_,
                         &footerCentre_, &footerRight_ })
        addAndMakeVisible(*label);

    for (auto* button : { &loadRom_, &loadSyx_, &saveAs_, &settings_, &about_, &previous_, &next_,
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
    for (int algorithm = 1; algorithm <= 32; ++algorithm)
        algorithm_.addItem(juce::String(algorithm), algorithm);
    algorithm_.setName("Algorithm");
    algorithm_.setTooltip("DX7 algorithm (1-32). Uses the existing automatable algorithm parameter.");
    algorithm_.onChange = [this] { updateVoiceValueLabels(); };
    addAndMakeVisible(algorithm_);
    algorithmView_.onOperatorSelected = [this](int op) { selectOperator(op); };
    settings_.setTooltip("Master tuning and MIDI input channel, stored in the DAW project.");
    settings_.onClick = [this] { showSettings(); };
    addChildComponent(performancePanel_);
    editTab_.onClick = [this] { showPerformance(false); };
    performanceTab_.onClick = [this] { showPerformance(true); };
    performanceTab_.setTooltip("Global controller range and assignments; saved with the DAW project.");
    utilityTab_.setClickingTogglesState(false);
    utilityTab_.setRadioGroupId(0);
    utilityTab_.setTooltip("Rename voice and copy/paste the selected operator.");
    utilityTab_.onClick = [this] { showUtilityMenu(); };

    static constexpr const char* bankNames[] =
        { "ROM1A", "ROM1B", "ROM2A", "ROM2B", "ROM3A", "ROM3B", "ROM4A", "ROM4B" };
    for (int i = 0; i < 8; ++i)
        bank_.addItem(bankNames[i], i + 1);
    bank_.addItem("USER (load copy)", 9);

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
                       juce::Colour(0xffc9c4b8));
        configureLabel(operatorKnobValues_[i], 12.0f, juce::Justification::centred,
                       juce::Colour(0xff68c7bb));
        operatorKnobCaptions_[i].setText(kKnobCaptions[i], juce::dontSendNotification);
        operatorKnobValues_[i].setColour(juce::Label::backgroundColourId,
                                         juce::Colour(0xff24221f));
        operatorKnobValues_[i].setColour(juce::Label::outlineColourId,
                                         juce::Colour(0xff71685b));
        operatorKnobs_[i].onValueChange = [this] { updateOperatorValueLabels(); };
        addAndMakeVisible(operatorKnobs_[i]);
        addAndMakeVisible(operatorKnobCaptions_[i]);
        addAndMakeVisible(operatorKnobValues_[i]);
    }

    for (std::size_t i = 0; i < envelopeFaders_.size(); ++i)
    {
        configureOperatorSlider(envelopeFaders_[i], true);
        envelopeFaders_[i].setName("Operator envelope " + juce::String(kEnvelopeCaptions[i]));
        configureLabel(envelopeCaptions_[i], 11.0f, juce::Justification::centred,
                       juce::Colour(0xffc9c4b8));
        configureLabel(envelopeValues_[i], 11.0f, juce::Justification::centred,
                       juce::Colour(0xff68c7bb));
        envelopeCaptions_[i].setText(kEnvelopeCaptions[i], juce::dontSendNotification);
        envelopeValues_[i].setColour(juce::Label::backgroundColourId,
                                     juce::Colour(0xff24221f));
        envelopeValues_[i].setColour(juce::Label::outlineColourId,
                                     juce::Colour(0xff71685b));
        envelopeFaders_[i].onValueChange = [this]
        {
            updateOperatorValueLabels();
            repaint(referenceRect(1080, 578, 312, 245));
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
                       juce::Colour(0xffc9c4b8));
        configureLabel(operatorScaleValues_[i], 10.0f, juce::Justification::centred,
                       juce::Colour(0xff68c7bb));
        operatorScaleCaptions_[i].setText(kScaleCaptions[i], juce::dontSendNotification);
        operatorScaleValues_[i].setColour(juce::Label::backgroundColourId,
                                          juce::Colour(0xff24221f));
        operatorScaleValues_[i].setColour(juce::Label::outlineColourId,
                                          juce::Colour(0xff71685b));
        operatorScaleKnobs_[i].onValueChange = [this] { updateOperatorValueLabels(); };
        addAndMakeVisible(operatorScaleKnobs_[i]);
        addAndMakeVisible(operatorScaleCaptions_[i]);
        addAndMakeVisible(operatorScaleValues_[i]);
    }
    operatorScaleValues_[0].setVisible(false);
    frequencyValue_.setJustificationType(juce::Justification::centred);
    frequencyValue_.setBorderSize(juce::BorderSize<int>(0));
    frequencyValue_.setColour(juce::Label::backgroundColourId, juce::Colour(0xff24221f));
    frequencyValue_.setColour(juce::Label::outlineColourId, juce::Colour(0xff71685b));

    for (std::size_t i = 0; i < pitchEnvelopeFaders_.size(); ++i)
    {
        configureOperatorSlider(pitchEnvelopeFaders_[i], true);
        pitchEnvelopeFaders_[i].setName("Pitch envelope " + juce::String(static_cast<int>(i)));
        configureLabel(pitchEnvelopeCaptions_[i], 9.0f, juce::Justification::centred,
                       juce::Colour(0xffc9c4b8));
        configureLabel(pitchEnvelopeValues_[i], 9.0f, juce::Justification::centred,
                       juce::Colour(0xff68c7bb));
        pitchEnvelopeCaptions_[i].setText(kEnvelopeCaptions[i], juce::dontSendNotification);
        pitchEnvelopeValues_[i].setColour(juce::Label::backgroundColourId,
                                          juce::Colour(0xff24221f));
        pitchEnvelopeValues_[i].setColour(juce::Label::outlineColourId,
                                          juce::Colour(0xff71685b));
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
        if (i == 1 || i == 7)
        {
            voiceKnobs_[i].setSliderStyle(juce::Slider::LinearVertical);
            voiceKnobs_[i].setSliderSnapsToMousePosition(true);
            voiceKnobs_[i].getProperties().set("vdx7SyncSwitch", true);
        }
        configureLabel(voiceKnobCaptions_[i], 8.5f, juce::Justification::centred,
                       juce::Colour(0xffc9c4b8));
        configureLabel(voiceKnobValues_[i], 9.0f, juce::Justification::centred,
                       juce::Colour(0xff68c7bb));
        voiceKnobCaptions_[i].setText(kVoiceKnobCaptions[i], juce::dontSendNotification);
        voiceKnobCaptions_[i].setBorderSize(juce::BorderSize<int>(0));
        voiceKnobValues_[i].setColour(juce::Label::backgroundColourId,
                                      juce::Colour(0xff24221f));
        voiceKnobValues_[i].setColour(juce::Label::outlineColourId,
                                      juce::Colour(0xff71685b));
        voiceKnobs_[i].onValueChange = [this] { updateVoiceValueLabels(); };
        addAndMakeVisible(voiceKnobs_[i]);
        addAndMakeVisible(voiceKnobCaptions_[i]);
        addAndMakeVisible(voiceKnobValues_[i]);
    }

    auto& parameters = processor_.parameters();
    algorithmAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        parameters, VDX7ParameterIDs::voiceParameter(VDX7VoiceData::VoiceParameter::algorithm), algorithm_);
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
    saveAs_.onClick = [this] { showSaveAsMenu(); };
    saveAs_.setTooltip("Save a patch to USER, or export a patch/bank SysEx file. Factory ROM is unchanged.");
    previous_.setTooltip("Previous program in the current bank (wraps 01-32)");
    next_.setTooltip("Next program in the current bank (wraps 01-32)");
    previous_.setName("Previous patch");
    next_.setName("Next patch");
    for (auto* button : { &previous_, &next_ })
        button->getProperties().set("vdx7LcdArrow", true);
    about_.onClick = [this]
    {
        juce::DialogWindow::LaunchOptions options;
        options.dialogTitle = "About VDX7 Mk 1.";
        options.dialogBackgroundColour = juce::Colour(0xff302d29);
        options.content.setOwned(new VDX7AboutPanel());
        options.componentToCentreAround = this;
        options.escapeKeyTriggersCloseButton = true;
        options.useNativeTitleBar = false;
        options.resizable = false;
        options.launchAsync();
    };

    bankCaption_.setText("Bank:", juce::dontSendNotification);
    programCaption_.setText("Program:", juce::dontSendNotification);
    for (auto* box : { &bank_, &program_ })
    {
        box->getProperties().set("vdx7LcdCombo", true);
        box->setColour(juce::ComboBox::backgroundColourId, juce::Colours::transparentBlack);
        box->setColour(juce::ComboBox::textColourId, juce::Colour(0xff273019));
        box->setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff273019));
        box->setColour(juce::ComboBox::arrowColourId, juce::Colour(0xff273019));
    }
    bank_.onChange = [this]
    {
        if (internalUiUpdate_) return;
        const int bankIndex = bank_.getSelectedId() - 1;
        if (bankIndex >= 0)
        {
            refresh(false);
            confirmReplacement([this, bankIndex]
            {
                if (bankIndex == 8)
                {
                    juce::String error;
                    if (!processor_.loadUserBank(VDX7AudioProcessor::userBankFile(), error))
                        showError("USER bank", error);
                }
                else processor_.selectFactoryBank(bankIndex);
            });
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
    repaint(referenceRect(34, 526, 1372, 310));
    algorithmView_.setState(algorithm_.getSelectedId(), selectedOperator_,
                            juce::roundToInt(voiceKnobs_[0].getValue()));
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

    algorithmView_.setState(algorithm_.getSelectedId(), selectedOperator_,
                            juce::roundToInt(voiceKnobs_[0].getValue()));
}

void VDX7AudioProcessorEditor::drawOperatorEnvelope(juce::Graphics& g)
{
    const float scaleY = static_cast<float>(getHeight()) / kReferenceHeight;
    const auto graph = referenceRect(1084, 585, 300, 239).toFloat();
    drawEnvelopeGrid(g, graph, scaleY);

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

    g.setColour(juce::Colour(0xff68c7bb));
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
    const auto graph = referenceRect(326, 394, 264, 112).toFloat();
    drawEnvelopeGrid(g, graph, scaleY);

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
    g.setColour(juce::Colour(0xff68c7bb));
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
            juce::jmax(4.0f, referenceSize * scale), style)));
    };

    setFont(status_, 12.0f);
    setFont(patch_, 29.0f, juce::Font::bold);
    setFont(bankCaption_, 15.5f);
    setFont(programCaption_, 14.0f);
    setFont(masterCaption_, 14.0f);
    setFont(masterValue_, 17.0f);
    setFont(pitchCaption_, 15.5f);
    setFont(modCaption_, 14.0f);
    setFont(outputCaption_, 19.0f);
    setFont(leftCaption_, 12.0f);
    setFont(rightCaption_, 12.0f);
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
    g.fillAll(juce::Colour(0xff211f1c));

    const auto enclosure = getLocalBounds().toFloat().reduced(9.0f * scaleY);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff474139), 0, 0,
        juce::Colour(0xff292622), 0, float(getHeight()), false));
    g.fillRoundedRectangle(enclosure, 7.0f * scaleY);
    g.setColour(juce::Colour(0xff71685b));
    g.drawRoundedRectangle(enclosure, 7.0f * scaleY, scaleY);

    const auto drawPanel = [this, &g](float x, float y, float width, float height)
    {
        const auto panel = referenceRect(x, y, width, height).toFloat();
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff312e28), panel.getTopLeft(),
            juce::Colour(0xff23221e), panel.getBottomLeft(), false));
        g.fillRoundedRectangle(panel, 5.0f);
        g.setColour(juce::Colour(0xff504b42));
        g.drawRoundedRectangle(panel, 5.0f, 1.0f);
    };

    drawPanel(34, 135, 920, 190);
    drawPanel(968, 135, 220, 190);
    drawPanel(1200, 135, 206, 391);
    drawPanel(34, 340, 1154, performanceVisible_ ? 210 : 185);
    if (performanceVisible_)
        drawPanel(34, 550, 1372, 286);
    else
        drawPanel(34, 526, 1372, 310);
    drawPanel(34, 838, 1372, 180);

    // Align actual ink/geometry, not font boxes: header buttons paint 1.5px
    // inside their bounds. Use that same visible lower edge for logo and text.
    const float headerBottom = about_.getBottom() - 1.5f;
    const float scaleX = float(getWidth()) / kReferenceWidth;
    const float logoWidth = 300.0f * scaleX;
    const float logoHeight = logoWidth * 381.0f / 1947.0f;
    // Three understated header accents reuse the section-divider tone and
    // span the full header inset. Keep them above the brand/action row.
    g.setColour(juce::Colour(0xff71685b));
    for (const float y : { 22.0f, 34.0f, 46.0f })
        g.fillRect(referenceRect(34, y, 1372, 1));
    if (wordmark_)
        wordmark_->drawWithin(g, { 44.0f * scaleX, headerBottom - logoHeight,
                                  logoWidth, logoHeight }, juce::RectanglePlacement::stretchToFit, 1.0f);
    g.setColour(juce::Colour(0xffbdb8ac));
    juce::GlyphArrangement modelMark;
    modelMark.addLineOfText(juce::Font(juce::FontOptions(52.0f * scaleY, juce::Font::bold)),
                            "Mk1.", 0.0f, 0.0f);
    const auto modelMarkInk = modelMark.getBoundingBox(0, -1, true);
    modelMark.draw(g, juce::AffineTransform::translation(
        361.0f * scaleX - modelMarkInk.getX(),
        headerBottom - modelMarkInk.getBottom() + 5.0f * scaleY));
    const auto headerText = [&g, scaleX, scaleY](const juce::String& text, float bottom,
                                                float size, int style)
    {
        juce::GlyphArrangement glyphs;
        glyphs.addLineOfText(juce::Font(juce::FontOptions(size * scaleY, style)), text, 0, 0);
        const auto ink = glyphs.getBoundingBox(0, -1, true);
        g.setColour(juce::Colour(0xffd2cdc1));
        glyphs.draw(g, juce::AffineTransform::translation(490.0f * scaleX - ink.getX(), bottom - ink.getBottom()));
    };
    headerText("HARDWARE EMULATION", headerBottom - 25.0f * scaleY, 19.0f, juce::Font::bold);
    headerText("Original firmware required.", headerBottom, 19.0f, juce::Font::plain);
    g.setColour(juce::Colour(0xff71685b));
    g.fillRect(referenceRect(34, 131, 1372, 1));
    g.setColour(juce::Colour(0xff141510));
    g.fillRoundedRectangle(referenceRect(412, 180, 525, 112).toFloat(), 5.0f);
    const auto lcd = referenceRect(422, 190, 505, 86).toFloat();
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xffc9d68e), lcd.getX(), lcd.getY(),
        juce::Colour(0xffa5b76e), lcd.getX(), lcd.getBottom(), false));
    g.fillRoundedRectangle(lcd, 3.0f);
    const auto footerBounds = referenceRect(18, 1030, 1404, 44).toFloat();
    juce::ColourGradient footerGradient(juce::Colour(0xff24221f), footerBounds.getX(),
                                        footerBounds.getCentreY(), juce::Colour(0xff24221f),
                                        footerBounds.getRight(), footerBounds.getCentreY(), false);
    footerGradient.addColour(0.5, juce::Colour(0xff302d29));
    g.setGradientFill(footerGradient);
    g.fillRoundedRectangle(footerBounds, 3.0f * scaleY);
    g.setColour(juce::Colour(0xff71685b));
    g.fillRect(footerBounds.withHeight(juce::jmax(1.0f, scaleY)));
    g.fillRect(referenceRect(18, 1077, 1404, 1));
    constexpr float outputContentOffsetY = 10.0f;
    g.drawImage(valueField_, referenceRect(1255, 467 + outputContentOffsetY, 96, 32).toFloat());

    g.setFont(juce::Font(juce::FontOptions(20.0f * scaleY)));
    g.setColour(juce::Colour(0xffeee9dc));
    g.drawText("VOICE", referenceRect(52, 145, 250, 30), juce::Justification::centredLeft);
    g.drawText(performanceVisible_ ? "PERFORMANCE" : "GLOBAL",
               referenceRect(52, 350, 250, 30), juce::Justification::centredLeft);
    g.drawText("ALGORITHM", referenceRect(980, 145, 126, 30), juce::Justification::centredLeft);

    g.setColour(juce::Colour(0xff71685b));
    g.fillRect(referenceRect(52, 176, 884, 1));
    g.fillRect(referenceRect(52, 382, 1118, 1));
    if (!performanceVisible_)
    {
        g.fillRect(referenceRect(52, 574, 1336, 1));
        g.fillRect(referenceRect(48, 720, 632, 1));
    }

    // Seven subtle Phillips fasteners sit at the four enclosure corners,
    // halfway down each side, and at the centre of the lower edge.
    for (const auto p : { juce::Point<float>(23.0f, 23.0f), juce::Point<float>(1417.0f, 23.0f),
                          juce::Point<float>(23.0f, 555.0f), juce::Point<float>(1417.0f, 555.0f),
                          juce::Point<float>(23.0f, 1087.0f), juce::Point<float>(720.0f, 1087.0f),
                          juce::Point<float>(1417.0f, 1087.0f) })
        drawChassisScrew(g, { p.x * scaleX, p.y * scaleY }, 7.0f * scaleY);

    // Raised metallic lip above the existing keyboard/wheel bay.
    g.setColour(juce::Colour(0xff89877e));
    g.fillRect(referenceRect(210, 862, 1184, 1.3f));
    g.setColour(juce::Colour(0xff151715));
    g.fillRect(referenceRect(210, 864, 1184, 2.0f));

    // Wheel scales follow the hardware reference: opposing pitch arrows
    // separated by zero, and a simple max/min modulation guide.
    const auto wheelScalePoint = [scaleX, scaleY](float x, float y)
    {
        return juce::Point<float>(x * scaleX, y * scaleY);
    };
    g.setColour(juce::Colour(0xffd0cec3));
    constexpr float pitchControlOffsetX = -6.0f;
    const auto pitchScalePoint = [&wheelScalePoint](float x, float y)
    {
        return wheelScalePoint(x + pitchControlOffsetX, y);
    };
    juce::Path pitchUp;
    pitchUp.startNewSubPath(pitchScalePoint(121.5f, 889));
    pitchUp.lineTo(pitchScalePoint(128, 927));
    pitchUp.lineTo(pitchScalePoint(115, 927));
    pitchUp.closeSubPath();
    g.fillPath(pitchUp);
    juce::Path pitchDown;
    pitchDown.startNewSubPath(pitchScalePoint(115, 951));
    pitchDown.lineTo(pitchScalePoint(128, 951));
    pitchDown.lineTo(pitchScalePoint(121.5f, 989));
    pitchDown.closeSubPath();
    g.fillPath(pitchDown);

    const float modulationScaleX = 195.5f * scaleX;
    g.drawLine(modulationScaleX, 889.0f * scaleY, modulationScaleX, 932.0f * scaleY,
               juce::jmax(1.0f, scaleY));
    g.drawLine(modulationScaleX, 947.0f * scaleY, modulationScaleX, 989.0f * scaleY,
               juce::jmax(1.0f, scaleY));
    g.drawLine(191.0f * scaleX, 889.0f * scaleY, 200.0f * scaleX, 889.0f * scaleY,
               juce::jmax(1.0f, scaleY));
    g.drawLine(191.0f * scaleX, 989.0f * scaleY, 200.0f * scaleX, 989.0f * scaleY,
               juce::jmax(1.0f, scaleY));

    g.setColour(juce::Colour(0xffd0cec3));
    g.setFont(juce::Font(juce::FontOptions(8.0f * scaleY)));
    g.drawFittedText("UP", referenceRect(105 + pitchControlOffsetX, 873, 33, 13), juce::Justification::centred, 1);
    g.drawFittedText("0", referenceRect(105 + pitchControlOffsetX, 933, 33, 13), juce::Justification::centred, 1);
    g.drawFittedText("DOWN", referenceRect(105 + pitchControlOffsetX, 993, 33, 13), juce::Justification::centred, 1);
    g.drawFittedText("MAX", referenceRect(181, 873, 29, 13), juce::Justification::centred, 1);
    g.drawFittedText("0", referenceRect(181, 933, 29, 13), juce::Justification::centred, 1);
    g.drawFittedText("MIN", referenceRect(181, 993, 29, 13), juce::Justification::centred, 1);

    if (performanceVisible_)
    {
        return;
    }

    g.setFont(juce::Font(juce::FontOptions(11.0f * scaleY)));
    g.setColour(juce::Colour(0xffc9c4b8));

    g.setColour(juce::Colour(0xff71685b));
    g.fillRect(referenceRect(594, 390, 1, 116));
    drawPitchEnvelope(g);
    drawOperatorEnvelope(g);
}

void VDX7AudioProcessorEditor::resized()
{
    updateResponsiveTypography();
    performancePanel_.setBounds(referenceRect(44, 392, 1352, 434));

    previous_.setBounds(referenceRect(431, 198, 32, 70));
    next_.setBounds(referenceRect(887, 198, 32, 70));
    loadRom_.setBounds(referenceRect(894, 76, 104, 46));
    loadSyx_.setBounds(referenceRect(1000, 76, 104, 46));
    saveAs_.setBounds(referenceRect(1106, 76, 104, 46));
    settings_.setBounds(referenceRect(1212, 76, 104, 46));
    about_.setBounds(referenceRect(1318, 76, 90, 46));
    for (auto* button : { &loadRom_, &loadSyx_, &saveAs_, &settings_, &about_ })
        button->getProperties().set("vdx7WideHeader", true);
    status_.setBounds(referenceRect(56, 242, 324, 64));

    editTab_.setBounds(referenceRect(54, 190, 100, 42));
    performanceTab_.setBounds(referenceRect(160, 190, 118, 42));
    utilityTab_.setBounds(referenceRect(284, 190, 100, 42));
    patch_.setBounds(referenceRect(472, 190, 412, 40));
    bankCaption_.setBounds(referenceRect(472, 238, 52, 24));
    bank_.setBounds(referenceRect(516, 238, 94, 24));
    programCaption_.setBounds(referenceRect(625, 238, 76, 24));
    program_.setBounds(referenceRect(693, 238, 72, 24));

    outputCaption_.setBounds(referenceRect(1210, 146, 186, 30));
    constexpr float outputContentOffsetY = 10.0f;
    leftCaption_.setBounds(referenceRect(1210, 178 + outputContentOffsetY, 58, 22));
    masterCaption_.setBounds(referenceRect(1270, 178 + outputContentOffsetY, 66, 22));
    rightCaption_.setBounds(referenceRect(1338, 178 + outputContentOffsetY, 58, 22));
    leftMeter_.setBounds(referenceRect(1220, 202 + outputContentOffsetY, 38, 232));
    masterVolume_.setBounds(referenceRect(1274, 202 + outputContentOffsetY, 58, 232));
    rightMeter_.setBounds(referenceRect(1348, 202 + outputContentOffsetY, 38, 232));
    masterValue_.setBounds(referenceRect(1255, 467 + outputContentOffsetY, 96, 32));
    algorithm_.setBounds(referenceRect(1109, 145, 68, 30));
    algorithmView_.setBounds(referenceRect(980, 180, 200, 124));
    pitchEnvelopeTitle_.setBounds(referenceRect(52, 384, 260, 20));
    voiceLfoTitle_.setBounds(referenceRect(602, 384, 260, 20));
    for (std::size_t i = 0; i < pitchEnvelopeFaders_.size(); ++i)
    {
        const float x = 52.0f + static_cast<float>(i) * 33.0f;
        pitchEnvelopeCaptions_[i].setBounds(referenceRect(x, 404, 28, 12));
        pitchEnvelopeFaders_[i].setBounds(referenceRect(x, 411, 28, 88));
        pitchEnvelopeValues_[i].setBounds(referenceRect(x, 491, 28, 18));
    }
    for (std::size_t i = 0; i < voiceKnobs_.size(); ++i)
    {
        const float x = 606.0f + static_cast<float>(i) * 56.0f;
        voiceKnobCaptions_[i].setBounds(referenceRect(x, 404, 48, 16));
        voiceKnobs_[i].setBounds(referenceRect(x, 426, 48, 48));
        voiceKnobValues_[i].setBounds(referenceRect(x, 486, 48, 18));
    }

    for (int op = 0; op < VDX7VoiceData::kOperatorCount; ++op)
        operatorTabs_[static_cast<std::size_t>(op)].setBounds(
            referenceRect(42.0f + op * 104.0f, 532, 96, 38));

    operatorTitle_.setBounds(referenceRect(52, 578, 260, 28));
    frequencyValue_.setBounds(referenceRect(52, 801, 92, 16));
    envelopeTitle_.setBounds(referenceRect(704, 578, 240, 28));
    for (std::size_t i = 0; i < operatorKnobs_.size(); ++i)
    {
        const float x = 48.0f + static_cast<float>(i) * 92.0f;
        operatorKnobCaptions_[i].setBounds(referenceRect(x, 606, 80, 18));
        operatorKnobs_[i].setBounds(referenceRect(x + 11.0f, 626, 58, 58));
        operatorKnobValues_[i].setBounds(referenceRect(x, 690, 80, 20));
    }
    for (std::size_t i = 0; i < operatorScaleKnobs_.size(); ++i)
    {
        const float x = 48.0f + static_cast<float>(i) * 106.0f;
        operatorScaleCaptions_[i].setBounds(referenceRect(x, 733, 92, 16));
        operatorScaleKnobs_[i].setBounds(referenceRect(x + 24.0f, 753, 44, 44));
        if (i == 0)
            operatorScaleKnobs_[i].setBounds(referenceRect(x + 17.0f, 753, 58, 44));
        operatorScaleValues_[i].setBounds(referenceRect(x, 801, 92, 16));
    }

    for (std::size_t i = 0; i < envelopeFaders_.size(); ++i)
    {
        const float x = 704.0f + static_cast<float>(i) * 47.0f;
        envelopeCaptions_[i].setBounds(referenceRect(x, 606, 36, 18));
        envelopeFaders_[i].setBounds(referenceRect(x, 626, 36, 169));
        envelopeValues_[i].setBounds(referenceRect(x, 799, 36, 20));
    }

    constexpr float pitchControlOffsetX = -6.0f;
    pitchCaption_.setBounds(referenceRect(48 + pitchControlOffsetX, 847, 72, 24));
    modCaption_.setBounds(referenceRect(124, 847, 72, 24));
    pitchWheel_.setBounds(referenceRect(64.75f + pitchControlOffsetX, 870, 40.5f, 138));
    modWheel_.setBounds(referenceRect(140.75f, 870, 40.5f, 138));
    keyboard_.setBounds(referenceRect(210, 870, 1184, 138));
    keyboard_.setKeyWidth(static_cast<float>(keyboard_.getWidth()) / 36.0f);
    keyboard_.setLowestVisibleKey(36);

    footerLeft_.setBounds(referenceRect(38, 1034, 300, 34));
    footerCentre_.setBounds(referenceRect(480, 1034, 480, 34));
    footerRight_.setBounds(referenceRect(1050, 1034, 350, 34));
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

    patch_.setText(juce::String(programIndex + 1).paddedLeft('0', 2) + "   " + patchName,
                   juce::dontSendNotification);

    bank_.setSelectedId(bankIndex >= 0 ? bankIndex + 1 : 0, juce::dontSendNotification);
    bank_.setTextWhenNothingSelected(loaded ? "CUSTOM" : "");
    program_.setSelectedId(programIndex + 1, juce::dontSendNotification);
    for (int i = 1; i <= 8; ++i) bank_.setItemEnabled(i, factories);
    bank_.setEnabled(loaded);
    program_.setEnabled(loaded);
    loadSyx_.setEnabled(loaded);
    saveAs_.setEnabled(loaded);
    algorithm_.setEnabled(loaded);
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
        if (performanceVisible_) performancePanel_.refresh();
        status_.setText(VDX7StatusPresentation::choose(processor_.getCriticalStatusText(),
                            processor_.hasUnexportedEdits(), processor_.getStatusText()),
                        juce::dontSendNotification);
        loadRom_.setTooltip(processor_.getRomPath());
    }

    internalUiUpdate_ = false;
}

void VDX7AudioProcessorEditor::showPerformance(bool visible)
{
    performanceVisible_ = visible;
    editTab_.setToggleState(!visible, juce::dontSendNotification);
    performanceTab_.setToggleState(visible, juce::dontSendNotification);
    const auto showEdit = [visible](auto& components)
    {
        for (auto& component : components) component.setVisible(!visible);
    };
    showEdit(operatorTabs_);
    showEdit(operatorKnobs_); showEdit(operatorKnobCaptions_); showEdit(operatorKnobValues_);
    showEdit(operatorScaleKnobs_); showEdit(operatorScaleCaptions_); showEdit(operatorScaleValues_);
    operatorScaleValues_[0].setVisible(false); // combined oscillator-mode/frequency label
    showEdit(envelopeFaders_); showEdit(envelopeCaptions_); showEdit(envelopeValues_);
    showEdit(pitchEnvelopeFaders_); showEdit(pitchEnvelopeCaptions_); showEdit(pitchEnvelopeValues_);
    showEdit(voiceKnobs_); showEdit(voiceKnobCaptions_); showEdit(voiceKnobValues_);
    for (auto* label : { &operatorTitle_, &frequencyValue_, &envelopeTitle_,
                         &pitchEnvelopeTitle_, &voiceLfoTitle_ })
        label->setVisible(!visible);
    performancePanel_.setVisible(visible);
    if (visible) performancePanel_.refresh();
    repaint();
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
        "This action can replace edited sounds. Cancel and use SAVE AS > Export Bank first "
        "to preserve all edited voices in a separate file. Your DAW project saves remain independent.",
        "Continue", "Cancel", nullptr,
        juce::ModalCallbackFunction::create([safe, action = std::move(action)](int result)
        {
            if (safe == nullptr) return;
            if (result != 0) action();
            safe->refresh(true);
        }));
}

void VDX7AudioProcessorEditor::showSaveAsMenu()
{
    VDX7UserBank::Voice patch;
    juce::String error;
    if (!processor_.captureUserPatch(patch, error)) { showError("Save failed", error); return; }
    const auto revision = processor_.getOperatorVoiceRevision();
    const int program = processor_.getCurrentProgram();
    const auto file = VDX7AudioProcessor::userBankFile();
    VDX7UserBank::Snapshot bank;
    const auto loaded = VDX7UserBank::load(file, bank);
    auto* dialog = new juce::AlertWindow("SAVE AS",
        "Save a copy of the captured patch. Global PERFORMANCE settings stay in the DAW project.\n"
        "USER destination: " + file.getFullPathName()
        + (loaded.failed() ? "\nUSER unavailable: " + loaded.getErrorMessage() : juce::String()),
        juce::MessageBoxIconType::NoIcon);
    dialog->addComboBox("action", { "Save Patch to USER bank", "Export Patch (.syx)...", "Export Bank (.syx)..." }, "Action:");
    dialog->addTextEditor("name", juce::String::fromUTF8(reinterpret_cast<const char*>(patch.data() + 118), 10).trimEnd(), "USER patch name:");
    dialog->getTextEditor("name")->setInputRestrictions(10);
    juce::StringArray slots;
    int firstEmpty = 0;
    bool foundEmpty = false;
    for (int i = 0; i < 32; ++i)
    {
        slots.add(juce::String(i + 1).paddedLeft('0', 2) + "  "
                  + (bank.occupied(i) ? bank.name(i) : "[empty]"));
        if (!foundEmpty && !bank.occupied(i)) { firstEmpty = i; foundEmpty = true; }
    }
    dialog->addComboBox("slot", slots, "USER destination slot:");
    dialog->getComboBoxComponent("slot")->setSelectedItemIndex(firstEmpty);
    dialog->addButton("Continue", 1, juce::KeyPress(juce::KeyPress::returnKey));
    dialog->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
    juce::Component::SafePointer<VDX7AudioProcessorEditor> safe(this);
    dialog->enterModalState(true, juce::ModalCallbackFunction::create(
        [safe, dialog, file, bank, patch, loaded, revision, program](int result)
        {
            if (safe == nullptr || result != 1) return;
            const int action = dialog->getComboBoxComponent("action")->getSelectedItemIndex();
            if (action == 1 || action == 2)
            {
                VDX7UserBank::Voice current;
                juce::String error;
                if (!safe->processor_.captureUserPatch(current, error) || current != patch
                    || safe->processor_.getCurrentProgram() != program
                    || safe->processor_.getOperatorVoiceRevision() != revision)
                { safe->showError("Voice changed", "The working voice/bank changed. Reopen SAVE AS to export."); return; }
                safe->chooseExport(action == 2);
                return;
            }
            if (loaded.failed()) { safe->showError("USER bank unavailable", loaded.getErrorMessage()); return; }
            const int slot = dialog->getComboBoxComponent("slot")->getSelectedItemIndex();
            const auto name = dialog->getTextEditorContents("name");
            auto write = [safe, file, bank, patch, slot, name](bool confirmed)
            {
                if (safe == nullptr) return;
                auto outcome = file.getParentDirectory().createDirectory();
                if (outcome.wasOk()) outcome = VDX7UserBank::savePatch(file, bank, slot, patch, name, confirmed);
                if (outcome.failed()) safe->showError("USER save failed", outcome.getErrorMessage());
                else juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,
                    "Patch saved", "Saved a copy to USER slot " + juce::String(slot + 1)
                    + ".\nSelect USER (load copy) in the LCD bank menu to play it.\n"
                    "The current working bank and its edit markers are unchanged.");
                safe->refresh(true);
            };
            if (!bank.occupied(slot)) { write(false); return; }
            juce::AlertWindow::showOkCancelBox(juce::MessageBoxIconType::WarningIcon,
                "Overwrite USER patch?", "Replace slot " + juce::String(slot + 1) + " (" + bank.name(slot)
                + ") with " + name + "? Other slots will be kept.", "Overwrite", "Cancel", nullptr,
                juce::ModalCallbackFunction::create([write](int answer) { if (answer != 0) write(true); }));
        }), true);
}

void VDX7AudioProcessorEditor::showUtilityMenu()
{
    juce::PopupMenu menu;
    menu.addItem(1, "Rename voice...");
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
                case 4: safe->processor_.copyOperator(op); break;
                case 5: safe->processor_.pasteOperator(op); break;
                default: break;
            }
            safe->refresh(true);
        });
}

void VDX7AudioProcessorEditor::showSettings()
{
    static SettingsLookAndFeel settingsLookAndFeel;
    const int initial = processor_.getMasterTune();
    const int initialChannel = processor_.getMidiInputChannel();
    const auto initialCorrection = processor_.getMonoCorrectionStatus();
    constexpr std::array<int, 5> guiScalePercentages { 50, 75, 100, 125, 150 };
    const float currentPercent = static_cast<float>(getWidth()) * 100.0f / 1200.0f;
    int initialScaleIndex = 0;
    for (int i = 1; i < static_cast<int>(guiScalePercentages.size()); ++i)
        if (std::abs(guiScalePercentages[static_cast<std::size_t>(i)] - currentPercent)
            < std::abs(guiScalePercentages[static_cast<std::size_t>(initialScaleIndex)] - currentPercent))
            initialScaleIndex = i;
    const juce::String correctionStatus = initialCorrection.active ? "Active (verified firmware)"
        : initialCorrection.requested ? (initialCorrection.loaded ? "Unavailable for this firmware: native behavior"
                                                                   : "Waiting for compatible firmware")
                                      : "Native firmware behavior";
    auto* dialog = new juce::AlertWindow("SETTINGS", {}, juce::MessageBoxIconType::NoIcon);
    dialog->setLookAndFeel(&settingsLookAndFeel);
    juce::StringArray settingsInfoRows
    {
        "Master tuning: -256 to +255 firmware units (not cents). 0 = default tuning.",
        "Saved in the DAW project, not voice/bank SysEx.",
        "Channel changes release held notes/sustain on the next audio block.",
        "On-screen keyboard and bank SysEx import are not channel-filtered.",
        "Advanced MONO compatibility is normally best left at Native firmware.",
        "Both modes accept MIDI Notes 12-120 only.",
        "Changing MONO correction restarts the engine and stops playing notes.",
        "MONO correction: " + correctionStatus
    };
    dialog->addCustomComponent(new SettingsInfoComponent(std::move(settingsInfoRows)));
    juce::StringArray guiScaleChoices;
    for (const auto percentage : guiScalePercentages)
        guiScaleChoices.add(juce::String(percentage) + "%");
    dialog->addTextEditor("tuning", juce::String(initial), "Master tuning:");
    dialog->getTextEditor("tuning")->setInputRestrictions(4, "-0123456789");
    dialog->addComboBox("guiScale", guiScaleChoices, "GUI size:");
    auto* scaleCombo = dialog->getComboBoxComponent("guiScale");
    scaleCombo->getProperties().set("vdx7SettingsCombo", true);
    scaleCombo->setLookAndFeel(&settingsLookAndFeel);
    scaleCombo->setSelectedItemIndex(initialScaleIndex);
    juce::StringArray channels {"OMNI (all channels)"};
    for (int channel = 1; channel <= 16; ++channel) channels.add(juce::String(channel));
    dialog->addComboBox("channel", channels, "Host MIDI input:");
    auto* channelCombo = dialog->getComboBoxComponent("channel");
    channelCombo->getProperties().set("vdx7SettingsCombo", true);
    channelCombo->setLookAndFeel(&settingsLookAndFeel);
    channelCombo->setSelectedItemIndex(initialChannel);
    dialog->addComboBox("monoCorrection", {"Native firmware (recommended)", "Correct MONO Note 0 (advanced)"},
                        "Advanced MONO engine mode:");
    auto* correctionCombo = dialog->getComboBoxComponent("monoCorrection");
    correctionCombo->getProperties().set("vdx7SettingsCombo", true);
    correctionCombo->setLookAndFeel(&settingsLookAndFeel);
    correctionCombo->setSelectedItemIndex(initialCorrection.requested ? 1 : 0);
    dialog->addButton("Apply", 1, juce::KeyPress(juce::KeyPress::returnKey));
    dialog->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
    juce::Component::SafePointer<VDX7AudioProcessorEditor> safe(this);
    dialog->enterModalState(true, juce::ModalCallbackFunction::create(
            [safe, dialog, initial, initialChannel, initialCorrection](int result)
        {
            if (dialog->getNumCustomComponents() > 0)
                delete dialog->removeCustomComponent(0);
            if (safe == nullptr || result != 1) return;
            const auto text = dialog->getTextEditorContents("tuning").trim();
            const auto digits = text.startsWithChar('-') ? text.substring(1) : text;
            if (digits.isEmpty() || !digits.containsOnly("0123456789")
                || text.getIntValue() < -256 || text.getIntValue() > 255)
            { safe->showError("Invalid tuning", "Enter an integer from -256 to 255."); return; }
            if (safe->processor_.getMasterTune() != initial
                || safe->processor_.getMidiInputChannel() != initialChannel
                || safe->processor_.getMonoCorrectionStatus().requested != initialCorrection.requested)
            { safe->showError("Settings changed", "Settings changed while this dialog was open. Reopen SETTINGS."); return; }
            if (text.getIntValue() != initial && !safe->processor_.setMasterTuneFromUi(text.getIntValue()))
            { safe->showError("Tuning unavailable", "Load compatible firmware before applying settings."); return; }
            safe->processor_.setMidiInputChannelFromUi(dialog->getComboBoxComponent("channel")->getSelectedItemIndex());
            if (!safe->processor_.setMonoCorrectionFromUi(
                    dialog->getComboBoxComponent("monoCorrection")->getSelectedItemIndex() == 1))
            {
                safe->showError("Correction unavailable", "A project restore may be in progress. Reopen SETTINGS.");
                return;
            }
            constexpr std::array<int, 5> scaleWidths { 600, 900, 1200, 1500, 1800 };
            constexpr std::array<int, 5> scaleHeights { 463, 694, 925, 1156, 1388 };
            const int scaleIndex = juce::jlimit(0, 4,
                dialog->getComboBoxComponent("guiScale")->getSelectedItemIndex());
            safe->setSize(scaleWidths[static_cast<std::size_t>(scaleIndex)],
                          scaleHeights[static_cast<std::size_t>(scaleIndex)]);
        }), true);
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
    VDX7AudioProcessor::SyxExportSnapshot snapshot;
    juce::String error;
    if (!processor_.captureSyxExportSnapshot(entireBank, snapshot, error))
    {
        showError("Save failed", error);
        return;
    }
    chooser_ = std::make_unique<juce::FileChooser>(
        entireBank ? "Save current 32-voice bank" : "Save current voice",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile(
            entireBank ? "VDX7-bank.syx" : juce::File::createLegalFileName(processor_.getCurrentPatchName()) + ".syx"),
        "*.syx");
    chooser_->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                          | juce::FileBrowserComponent::warnAboutOverwriting,
        [this, snapshot = std::move(snapshot)](const juce::FileChooser& chooser)
        {
            const auto file = chooser.getResult();
            if (file == juce::File()) return;
            juce::String error;
            if (!processor_.exportSyxSnapshot(file, snapshot, error)) showError("Save failed", error);
            refresh(true);
        });
}

void VDX7AudioProcessorEditor::showError(const juce::String& title, const juce::String& message)
{
    juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, title, message);
}
