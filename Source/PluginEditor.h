#pragma once
#include "VDX7AlgorithmView.h"

#include <JuceHeader.h>
#include <array>

#include "PluginProcessor.h"
#include "VDX7LookAndFeel.h"

class VDX7Keyboard final : public juce::MidiKeyboardComponent
{
public:
    explicit VDX7Keyboard(juce::MidiKeyboardState&);
    void paintOverChildren(juce::Graphics&) override;

    void drawWhiteNote(int midiNoteNumber, juce::Graphics&, juce::Rectangle<float> area,
                       bool isDown, bool isOver, juce::Colour lineColour,
                       juce::Colour textColour) override;
    void drawBlackNote(int midiNoteNumber, juce::Graphics&, juce::Rectangle<float> area,
                       bool isDown, bool isOver, juce::Colour noteFillColour) override;

private:
    juce::Image whiteNormal_;
    juce::Image whitePressed_;
    juce::Image blackNormal_;
    juce::Image blackPressed_;
};
class VDX7WheelSlider final : public juce::Slider
{
public:
    explicit VDX7WheelSlider(bool springToCentre);
    void mouseUp(const juce::MouseEvent&) override;

private:
    bool springToCentre_ = false;
};

class VDX7LevelMeter final : public juce::Component
{
public:
    VDX7LevelMeter();
    void setLevel(float newLevel);
    void paint(juce::Graphics&) override;

private:
    float level_ = 0.0f;
    juce::Image rail_;
    juce::Image ledOff_;
    juce::Image ledGreen_;
    juce::Image ledYellow_;
    juce::Image ledRed_;
};

class VDX7AudioProcessorEditor final : public juce::AudioProcessorEditor,
                                        private juce::Timer
{
public:
    explicit VDX7AudioProcessorEditor(VDX7AudioProcessor&);
    ~VDX7AudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    juce::Rectangle<int> referenceRect(float x, float y, float width, float height) const;
    void configureLabel(juce::Label&, float fontSize, juce::Justification,
                        juce::Colour colour = juce::Colour(0xffe6eef0));
    void configureTab(juce::TextButton&, int radioGroup);
    void configureOperatorSlider(juce::Slider&, bool isFader);
    void selectOperator(int operatorIndex);
    void bindSelectedOperatorParameters();
    void updateOperatorValueLabels();
    void updateVoiceValueLabels();
    void drawOperatorEnvelope(juce::Graphics&);
    void drawPitchEnvelope(juce::Graphics&);
    void updateResponsiveTypography();
    void timerCallback() override;
    void refresh(bool refreshMetadata);
    void chooseRom();
    void chooseSyx();
    void showUtilityMenu();
    void chooseExport(bool entireBank);
    void renameVoice();
    void confirmReplacement(std::function<void()> action);
    void showError(const juce::String& title, const juce::String& message);

    VDX7AudioProcessor& processor_;
    VDX7LookAndFeel lookAndFeel_;
    VDX7Keyboard keyboard_;

    juce::Image chassis_;
    juce::Image wordmark_;
    juce::Image lcdFrame_;
    juce::Image panel_;
    juce::Image valueField_;
    juce::Image envelopeGrid_;
    juce::Image divider_;

    juce::Label mkLabel_;
    juce::Label hardwareLabel_;
    juce::Label firmwareLabel_;
    juce::Label headerPatch_;
    juce::Label status_;
    juce::Label patch_;
    juce::Label bankCaption_;
    juce::Label programCaption_;
    juce::Label masterCaption_;
    juce::Label masterValue_;
    juce::Label pitchCaption_;
    juce::Label modCaption_;
    juce::Label outputCaption_;
    juce::Label leftCaption_;
    juce::Label rightCaption_;
    juce::Label algorithmDisplay_;
    VDX7AlgorithmView algorithmView_;
    juce::Label operatorTitle_;
    juce::Label frequencyValue_;
    juce::Label envelopeTitle_;
    juce::Label pitchEnvelopeTitle_;
    juce::Label voiceLfoTitle_;
    juce::Label footerLeft_;
    juce::Label footerCentre_;
    juce::Label footerRight_;

    juce::TextButton loadRom_ { "LOAD ROM" };
    juce::TextButton loadSyx_ { "LOAD SYX" };
    juce::TextButton settings_ { "SETTINGS" };
    juce::TextButton about_ { "ABOUT" };
    juce::TextButton previous_ { "<" };
    juce::TextButton next_ { ">" };
    juce::TextButton editTab_ { "EDIT" };
    juce::TextButton performanceTab_ { "PERFORMANCE" };
    juce::TextButton utilityTab_ { "UTILITY" };
    std::array<juce::TextButton, VDX7VoiceData::kOperatorCount> operatorTabs_;
    juce::ComboBox bank_;
    juce::ComboBox program_;

    juce::Slider masterVolume_;
    VDX7WheelSlider pitchWheel_ { true };
    VDX7WheelSlider modWheel_ { false };
    VDX7LevelMeter leftMeter_;
    VDX7LevelMeter rightMeter_;
    std::array<juce::Slider, 7> operatorKnobs_;
    std::array<juce::Label, 7> operatorKnobCaptions_;
    std::array<juce::Label, 7> operatorKnobValues_;
    std::array<juce::Slider, 8> envelopeFaders_;
    std::array<juce::Label, 8> envelopeCaptions_;
    std::array<juce::Label, 8> envelopeValues_;
    std::array<juce::Slider, 6> operatorScaleKnobs_;
    std::array<juce::Label, 6> operatorScaleCaptions_;
    std::array<juce::Label, 6> operatorScaleValues_;
    std::array<juce::Slider, 8> pitchEnvelopeFaders_;
    std::array<juce::Label, 8> pitchEnvelopeCaptions_;
    std::array<juce::Label, 8> pitchEnvelopeValues_;
    std::array<juce::Slider, 11> voiceKnobs_;
    std::array<juce::Label, 11> voiceKnobCaptions_;
    std::array<juce::Label, 11> voiceKnobValues_;

    std::unique_ptr<SliderAttachment> masterVolumeAttachment_;
    std::unique_ptr<SliderAttachment> pitchWheelAttachment_;
    std::unique_ptr<SliderAttachment> modWheelAttachment_;
    std::array<std::unique_ptr<SliderAttachment>, 7> operatorKnobAttachments_;
    std::array<std::unique_ptr<SliderAttachment>, 8> envelopeAttachments_;
    std::array<std::unique_ptr<SliderAttachment>, 6> operatorScaleAttachments_;
    std::array<std::unique_ptr<SliderAttachment>, 8> pitchEnvelopeAttachments_;
    std::array<std::unique_ptr<SliderAttachment>, 11> voiceKnobAttachments_;
    std::unique_ptr<juce::FileChooser> chooser_;

    bool internalUiUpdate_ = false;
    int metadataRefreshCounter_ = 0;
    int cpuTextRefreshCounter_ = 0;
    bool cpuDisplayInitialised_ = false;
    float displayedLeftPeak_ = 0.0f;
    float displayedRightPeak_ = 0.0f;
    double displayedCpuPercent_ = 0.0;
    int selectedOperator_ = 0;
    uint32_t lastOperatorVoiceRevision_ = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VDX7AudioProcessorEditor)
};
