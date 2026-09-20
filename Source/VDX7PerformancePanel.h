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
        for (int field = 0; field < 2; ++field)
        {
            auto& box = bend_[field];
            box.setName(field == 0 ? "Pitch bend range" : "Pitch bend step");
            for (int value = 0; value <= 12; ++value)
                box.addItem(juce::String(value) + (field == 1 && value == 0 ? " (continuous)" : " st"), value + 1);
            box.setTooltip("Firmware pitch bend. Nonzero STEP uses firmware stepped bending; RANGE applies to continuous mode. Saved in the project, not SysEx.");
            box.onChange = [this, field] { processor_.setPitchBendSettingFromUi(field, bend_[field].getSelectedId() - 1); };
            addAndMakeVisible(box);
            bendLabels_[field].setText(box.getName(), juce::dontSendNotification);
            addAndMakeVisible(bendLabels_[field]);
        }
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
        const auto bend = processor_.getPitchBendSettings();
        for (int field = 0; field < 2; ++field)
            bend_[field].setSelectedId(bend[field] + 1, juce::dontSendNotification);
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
        const float scale = getHeight() / 320.0f;
        const int top = juce::roundToInt(70 * scale);
        for (int field = 0; field < 2; ++field)
        {
            const int x = juce::roundToInt(field * column + 12 * scale);
            bendLabels_[field].setBounds(x, 0, int(column - 24 * scale), int(25 * scale));
            bend_[field].setBounds(x, int(27 * scale), int(column - 24 * scale), int(28 * scale));
        }
        for (int c = 0; c < 4; ++c)
        {
            const int x = juce::roundToInt(c * column);
            const int w = juce::roundToInt(column);
            ranges_[c].setBounds(x + w / 2 - int(40 * scale), top + int(45 * scale),
                                 int(80 * scale), int(84 * scale));
            for (int f = 0; f < 3; ++f)
                assignments_[c * 3 + f].setBounds(x + int(18 * scale),
                    top + int((143 + 30 * f) * scale), w - int(36 * scale), int(26 * scale));
        }
    }

    void paint(juce::Graphics& g) override
    {
        constexpr const char* titles[] { "MOD WHEEL", "FOOT CONTROL", "BREATH CONTROL", "AFTERTOUCH" };
        const float column = getWidth() / 4.0f;
        const float scale = getHeight() / 320.0f;
        const float top = 70 * scale;
        for (int c = 0; c < 4; ++c)
        {
            auto bounds = juce::Rectangle<float>(c * column + 3, top, column - 6, float(getHeight()) - top);
            g.setColour(juce::Colour(0xff101b1e));
            g.fillRoundedRectangle(bounds, 4.0f);
            g.setColour(juce::Colour(0xffe6eef0));
            g.setFont(juce::Font(juce::FontOptions(juce::jmax(11.0f, 17.0f * scale))));
            g.drawText(titles[c], bounds.removeFromTop(32 * scale).reduced(12, 0),
                       juce::Justification::centredLeft);
            g.setFont(juce::Font(juce::FontOptions(juce::jmax(9.0f, 11.0f * scale))));
            g.drawText("RANGE", int(c * column), int(top + 30 * scale), int(column), int(18 * scale),
                       juce::Justification::centred);
        }
    }

private:
    VDX7AudioProcessor& processor_;
    std::array<juce::ComboBox, 2> bend_;
    std::array<juce::Label, 2> bendLabels_;
    std::array<juce::Slider, 4> ranges_;
    std::array<juce::ToggleButton, 12> assignments_;
};
