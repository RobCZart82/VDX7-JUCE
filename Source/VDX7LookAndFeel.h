#pragma once

#include <JuceHeader.h>

class VDX7LookAndFeel final : public juce::LookAndFeel_V4
{
public:
    VDX7LookAndFeel();

    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&,
                              bool highlighted, bool down) override;
    void drawButtonText(juce::Graphics&, juce::TextButton&, bool highlighted, bool down) override;
    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override;

    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
                          float sliderPos, float startAngle, float endAngle,
                          juce::Slider&) override;
    void drawLinearSlider(juce::Graphics&, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          juce::Slider::SliderStyle, juce::Slider&) override;

    void drawComboBox(juce::Graphics&, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox&) override;
    juce::Font getComboBoxFont(juce::ComboBox&) override;
    void positionComboBoxText(juce::ComboBox&, juce::Label&) override;
    void drawComboBoxTextWhenNothingSelected(juce::Graphics&, juce::ComboBox&, juce::Label&) override;

private:
    const juce::Image& buttonImage(bool isTab, bool active, bool highlighted, bool down) const;
    const juce::Image& knobImage(bool active, bool highlighted, bool down) const;
    const juce::Image& faderThumbImage(bool active, bool highlighted, bool down) const;

    juce::Image buttonNormal_;
    juce::Image buttonHover_;
    juce::Image buttonPressed_;
    juce::Image buttonActive_;
    juce::Image tabNormal_;
    juce::Image tabHover_;
    juce::Image tabPressed_;
    juce::Image tabActive_;
    juce::Image knobNormal_;
    juce::Image knobHover_;
    juce::Image knobPressed_;
    juce::Image knobActive_;
    juce::Image faderTrack_;
    juce::Image faderThumbNormal_;
    juce::Image faderThumbHover_;
    juce::Image faderThumbPressed_;
    juce::Image faderThumbActive_;
    juce::Image wheelBase_;
    juce::Image wheelMarker_;
};
