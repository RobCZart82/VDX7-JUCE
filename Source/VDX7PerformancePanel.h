#pragma once
#include "PluginProcessor.h"

// First functional slice of the approved PERFORMANCE layout. Only implemented
// firmware controls appear here; remaining play-mode controls are not fake knobs.
class VDX7PerformancePanel final : public juce::Component
{
public:
    explicit VDX7PerformancePanel(VDX7AudioProcessor& processor) : processor_(processor)
    {
        setName("Performance controllers");
        constexpr const char* assignments[] { "PITCH", "AMPLITUDE", "EG BIAS" };
        for (int c = 0; c < 4; ++c)
        {
            auto& range = ranges_[c];
            range.setName("Controller " + juce::String(c) + " range");
            range.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            range.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 64, 20);
            range.setRange(0, 99, 1);
            range.setTooltip("Controller range (0-99). Saved in the DAW project, not voice SysEx.");
            range.onValueChange = [this, c]
            {
                processor_.setControllerSettingFromUi(c, 0, juce::roundToInt(ranges_[c].getValue()));
            };
            addAndMakeVisible(range);
            for (int f = 0; f < 3; ++f)
            {
                auto& button = assignments_[c * 3 + f];
                button.setButtonText(assignments[f]);
                button.setName("Controller " + juce::String(c) + " assignment " + juce::String(f));
                button.onClick = [this, c, f]
                {
                    processor_.setControllerSettingFromUi(c, f + 1,
                        assignments_[c * 3 + f].getToggleState() ? 1 : 0);
                };
                addAndMakeVisible(button);
            }
        }
        refresh();
    }

    void refresh()
    {
        setEnabled(processor_.isRomLoaded());
        const auto settings = processor_.getControllerSettings();
        for (int c = 0; c < 4; ++c)
        {
            if (!ranges_[c].isMouseButtonDown())
                ranges_[c].setValue(settings[c * 4], juce::dontSendNotification);
            for (int f = 0; f < 3; ++f)
                assignments_[c * 3 + f].setToggleState(settings[c * 4 + f + 1] != 0,
                                                     juce::dontSendNotification);
        }
    }

    void resized() override
    {
        const float column = getWidth() / 4.0f;
        const float scale = getHeight() / 250.0f;
        for (int c = 0; c < 4; ++c)
        {
            const int x = juce::roundToInt(c * column);
            const int w = juce::roundToInt(column);
            ranges_[c].setBounds(x + w / 2 - int(40 * scale), int(45 * scale),
                                 int(80 * scale), int(84 * scale));
            for (int f = 0; f < 3; ++f)
                assignments_[c * 3 + f].setBounds(x + int(18 * scale),
                    int((143 + 30 * f) * scale), w - int(36 * scale), int(26 * scale));
        }
    }

    void paint(juce::Graphics& g) override
    {
        constexpr const char* titles[] { "MOD WHEEL", "FOOT CONTROL", "BREATH CONTROL", "AFTERTOUCH" };
        const float column = getWidth() / 4.0f;
        const float scale = getHeight() / 250.0f;
        for (int c = 0; c < 4; ++c)
        {
            auto bounds = juce::Rectangle<float>(c * column + 3, 0, column - 6, float(getHeight()));
            g.setColour(juce::Colour(0xff101b1e));
            g.fillRoundedRectangle(bounds, 4.0f);
            g.setColour(juce::Colour(0xffe6eef0));
            g.setFont(juce::Font(juce::FontOptions(juce::jmax(11.0f, 17.0f * scale))));
            g.drawText(titles[c], bounds.removeFromTop(32 * scale).reduced(12, 0),
                       juce::Justification::centredLeft);
            g.setFont(juce::Font(juce::FontOptions(juce::jmax(9.0f, 11.0f * scale))));
            g.drawText("RANGE", int(c * column), int(30 * scale), int(column), int(18 * scale),
                       juce::Justification::centred);
        }
    }

private:
    VDX7AudioProcessor& processor_;
    std::array<juce::Slider, 4> ranges_;
    std::array<juce::ToggleButton, 12> assignments_;
};
