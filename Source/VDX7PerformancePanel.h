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
        constexpr const char* playNames[] {"Play mode", "Portamento mode", "Glissando", "Portamento time"};
        for (int field = 0; field < 4; ++field)
        {
            auto& box = play_[field];
            box.setName(playNames[field]);
            if (field == 0) box.addItemList({"POLY", "MONO"}, 1);
            else if (field == 1) box.addItemList({"Retain", "Follow"}, 1);
            else if (field == 2) box.addItemList({"OFF", "ON"}, 1);
            else for (int n = 0; n < 100; ++n) box.addItem(juce::String(n), n + 1);
            box.setTooltip(field == 0 ? "Changing play mode ends sounding notes in this instance. Saved with the project."
                : "Firmware portamento; CC65 controls the pedal. Time 0 is immediate. Saved with the project, not voice SysEx.");
            box.onChange = [this, field]
            {
                if (!processor_.setPlaySettingFromUi(field, play_[field].getSelectedId() - 1))
                    juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                        "Performance setting", "The firmware could not apply this setting. Please retry after playback stops.");
                refresh();
            };
            addAndMakeVisible(box);
            playLabels_[field].setText(playNames[field], juce::dontSendNotification);
            addAndMakeVisible(playLabels_[field]);
        }
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
        const auto play = processor_.getPlaySettings();
        if (lastMono_ != play[0])
        {
            lastMono_ = play[0];
            play_[1].changeItemText(1, lastMono_ ? "Fingered" : "Retain");
            play_[1].changeItemText(2, lastMono_ ? "Full time" : "Follow");
        }
        for (int field = 0; field < 4; ++field)
            play_[field].setSelectedId(play[field] + 1, juce::dontSendNotification);
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
        const float scale = getHeight() / 394.0f;
        const int top = juce::roundToInt(144 * scale);
        const float headerColumn = getWidth() * 0.83f / 3;
        for (int field = 0; field < 2; ++field)
        {
            const int x = juce::roundToInt(field * headerColumn + 12 * scale);
            bendLabels_[field].setBounds(x, 0, int(headerColumn - 24 * scale), int(25 * scale));
            bend_[field].setBounds(x, int(27 * scale), int(headerColumn - 24 * scale), int(28 * scale));
        }
        for (int field = 0; field < 4; ++field)
        {
            const int cell = field + 2;
            const int x = juce::roundToInt((cell % 3) * headerColumn + 12 * scale);
            const int y = juce::roundToInt((cell / 3) * 64 * scale);
            playLabels_[field].setBounds(x, y, int(headerColumn - 24 * scale), int(25 * scale));
            play_[field].setBounds(x, y + int(27 * scale), int(headerColumn - 24 * scale), int(28 * scale));
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
        const float scale = getHeight() / 394.0f;
        const float top = 144 * scale;
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
    std::array<juce::ComboBox, 4> play_;
    std::array<juce::Label, 4> playLabels_;
    int lastMono_ = -1;
    std::array<juce::ComboBox, 2> bend_;
    std::array<juce::Label, 2> bendLabels_;
    std::array<juce::Slider, 4> ranges_;
    std::array<juce::ToggleButton, 12> assignments_;
};
