#include "PluginEditor.h"
#include "VDX7AboutPanel.h"
#include "VDX7GuiScale.h"

#include <array>
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace
{
void require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

void checkWhiteKeyHover()
{
    juce::MidiKeyboardState state;
    VDX7Keyboard keyboard(state);
    for (int factor : { 1, 2 })
    {
        juce::Image normal(juce::Image::ARGB, 40 * factor, 138 * factor, true);
        juce::Image hover(juce::Image::ARGB, 40 * factor, 138 * factor, true);
        for (bool over : { false, true })
        {
            juce::Graphics g(over ? hover : normal);
            g.addTransform(juce::AffineTransform::scale(float(factor)));
            keyboard.drawWhiteNote(60, g, { 0, 0, 40, 138 }, false, over, {}, {});
        }
        for (int y = 0; y < 138 * factor; ++y)
            for (int x = 0; x < 40 * factor; ++x)
                if (y < 11 * factor || y >= 127 * factor)
                    require(normal.getPixelAt(x, y) == hover.getPixelAt(x, y),
                            "white hover stays off key margins");
        require(normal.getPixelAt(20 * factor, 70 * factor) != hover.getPixelAt(20 * factor, 70 * factor),
                "white key retains visible hover feedback");
    }
}
}

int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    try
    {
        checkWhiteKeyHover();
        for (std::size_t i = 0; i < VDX7GuiScale::presets.size(); ++i)
        {
            const auto& preset = VDX7GuiScale::presets[i];
            require(VDX7GuiScale::indexForWidth(preset.width) == static_cast<int>(i),
                    "each fixed Settings width selects its matching GUI size");
            const auto& selected = VDX7GuiScale::atIndex(static_cast<int>(i));
            require(selected.percentage == preset.percentage && selected.width == preset.width
                    && selected.height == preset.height,
                    "fixed Settings sizes keep their approved dimensions");
        }
        require(VDX7GuiScale::indexForWidth(750) == 0
                && VDX7GuiScale::indexForWidth(1050) == 1
                && VDX7GuiScale::indexForWidth(1350) == 2,
                "Settings size selection keeps nearest-preset tie behavior");

        VDX7AboutPanel about;
        require(about.hasVectorLogos(), "About loads the VDX7, GYR and signature vectors");
        for (auto* child : about.getChildren())
            require(about.getLocalBounds().contains(child->getBounds()),
                    "About controls remain inside their original panel");
        require(about.createComponentSnapshot(about.getLocalBounds()).isValid(),
                "About panel renders its vector artwork");
        {
            VDX7WheelSlider pitch(true);
            VDX7WheelSlider mod(false);
            pitch.setRange(0.0, 1.0, 0.001);
            mod.setRange(0.0, 1.0, 0.001);
            pitch.setSize(40, 140);
            mod.setSize(40, 140);
            pitch.setValue(0.5, juce::dontSendNotification);
            mod.setValue(0.5, juce::dontSendNotification);

            const auto now = juce::Time::getCurrentTime();
            const auto sendScroll = [now](juce::Slider& slider)
            {
                const juce::MouseEvent event {
                    juce::Desktop::getInstance().getMainMouseSource(), {10.0f, 10.0f},
                    juce::ModifierKeys {}, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                    &slider, &slider, now, {10.0f, 10.0f}, now, 0, false
                };
                slider.mouseWheelMove(event, {0.0f, 1.0f, false, true, false});
            };

            require(!pitch.isScrollWheelEnabled(), "pitch wheel ignores scroll/trackpad input");
            require(mod.isScrollWheelEnabled(), "mod wheel retains scroll/trackpad input");
            sendScroll(pitch);
            sendScroll(mod);
            require(std::abs(pitch.getValue() - 0.5) < 0.0001,
                    "pitch scroll cannot leave a persistent bend");
            require(std::abs(mod.getValue() - 0.5) > 0.0001,
                    "mod scroll continues to change and retain its value");

            require(pitch.keyPressed(juce::KeyPress(juce::KeyPress::upKey)),
                    "pitch wheel remains adjustable from the keyboard");
            require(std::abs(pitch.getValue() - 0.501) < 0.0001,
                    "keyboard input still changes pitch-wheel value");

            pitch.setValue(0.75, juce::sendNotificationSync);
            const juce::MouseEvent release {
                juce::Desktop::getInstance().getMainMouseSource(), {10.0f, 10.0f},
                juce::ModifierKeys {}, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                &pitch, &pitch, now, {10.0f, 10.0f}, now, 0, false
            };
            pitch.mouseUp(release);
            require(std::abs(pitch.getValue()) < 0.0001,
                    "pitch drag release still springs back to centre");
        }

        VDX7AudioProcessor processor(false);
        std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
        require(editor != nullptr, "editor creation without firmware");
        require(!editor->isResizable(), "host must not resize the editor window");
        int saveAsButtons = 0;
        bool algorithmDisabled = false;
        bool hasCornerResizer = false;
        juce::TextButton* editTab = nullptr;
        juce::TextButton* performanceTab = nullptr;
        VDX7PerformancePanel* performancePanel = nullptr;
        for (auto* child : editor->getChildren())
        {
            if (auto* button = dynamic_cast<juce::TextButton*>(child))
            {
                if (button->getButtonText() == "EDIT") editTab = button;
                if (button->getButtonText() == "PERFORMANCE") performanceTab = button;
                if (button->getButtonText() == "SAVE AS...")
                {
                    ++saveAsButtons;
                    require(!button->isEnabled(), "Save As is disabled without firmware");
                }
            }
            if (child->getName() == "Performance controllers")
                performancePanel = dynamic_cast<VDX7PerformancePanel*>(child);
            if (auto* box = dynamic_cast<juce::ComboBox*>(child))
                if (box->getName() == "Algorithm")
                {
                    algorithmDisabled = true;
                    require(!box->isEnabled(), "algorithm selector is disabled without firmware");
                }
            hasCornerResizer |= dynamic_cast<juce::ResizableCornerComponent*>(child) != nullptr;
        }
        require(saveAsButtons == 1, "one Save As control is present without firmware");
        require(algorithmDisabled, "algorithm selector is present without firmware");
        require(!hasCornerResizer, "bottom-right drag resizer is absent");
        require(editTab && performanceTab && performancePanel,
                "EDIT, PERFORMANCE and its panel are present");
        require(editTab->getToggleState() && !performanceTab->getToggleState()
                && !performancePanel->isVisible(), "editor opens in EDIT mode");

        constexpr std::array<const char*, 6> performanceComboNames
        {{
            "Play mode", "Portamento mode", "Glissando", "Portamento time",
            "Pitch bend range", "Pitch bend step"
        }};
        std::array<int, performanceComboNames.size()> performanceCombos {};
        int controllerRanges = 0;
        int controllerAssignments = 0;
        for (auto* child : performancePanel->getChildren())
        {
            if (auto* combo = dynamic_cast<juce::ComboBox*>(child))
                for (std::size_t i = 0; i < performanceComboNames.size(); ++i)
                    if (combo->getName() == performanceComboNames[i]) ++performanceCombos[i];
            if (auto* slider = dynamic_cast<juce::Slider*>(child))
                if (slider->getName().startsWith("Controller ")
                    && slider->getName().endsWith(" range")) ++controllerRanges;
            if (auto* button = dynamic_cast<juce::ToggleButton*>(child))
                if (button->getName().startsWith("Controller ")
                    && button->getName().contains(" assignment ")) ++controllerAssignments;
        }
        for (const auto count : performanceCombos)
            require(count == 1, "each named PERFORMANCE selector exists exactly once");
        require(controllerRanges == 4, "all four PERFORMANCE controller range controls exist");
        require(controllerAssignments == 12, "all twelve PERFORMANCE assignment switches exist");

        const juce::Colour accent(0xff71685b);
        constexpr std::array<float, 3> decorationY { 22.0f, 34.0f, 46.0f };
        for (const auto& preset : VDX7GuiScale::presets)
        {
            const int width = preset.width;
            const int height = preset.height;
            editor->setSize(width, height);
            require(editor->getWidth() == width && editor->getHeight() == height,
                    "editor accepts each fixed Settings preset size");
            for (auto* child : editor->getChildren())
                require(editor->getLocalBounds().contains(child->getBounds()),
                        "GUI child bounds remain inside each fixed preset size");
            const auto image = editor->createComponentSnapshot(editor->getLocalBounds());
            require(image.isValid(), "header snapshot at supported scale");
            const float scaleX = float(width) / 1440.0f;
            const float scaleY = float(height) / 1110.0f;
            const int removedSeparatorX = juce::roundToInt(594.0f * scaleX);
            const int removedSeparatorY = juce::roundToInt(450.0f * scaleY);
            require(image.getPixelAt(removedSeparatorX, removedSeparatorY).getARGB() != accent.getARGB(),
                    "unneeded separator beside the global curve display is absent");
            const int operatorSeparatorX = juce::roundToInt(692.0f * scaleX);
            const int operatorSeparatorY = juce::roundToInt(600.0f * scaleY);
            require(image.getPixelAt(operatorSeparatorX, operatorSeparatorY).getARGB() == accent.getARGB(),
                    "operator controls and envelope sliders have a section separator");
            // The top accent now has an intentional gap for the centre screw.
            const int headerX = juce::roundToInt(100.0f * scaleX);
            const int dividerX = juce::roundToInt(100.0f * scaleX);
            const int dividerY = juce::roundToInt(176.0f * scaleY);
            require(image.getPixelAt(dividerX, dividerY).getARGB() == accent.getARGB(),
                    "header accent test uses the existing section-divider color");

            for (const auto y : decorationY)
            {
                const int pixelY = static_cast<int>(std::floor(y * scaleY));
                require(image.getPixelAt(headerX, pixelY).getARGB() == accent.getARGB(),
                        "all three header accents are visible in divider color");
            }
            if (argc == 2 && width == 1200)
            {
                juce::FileOutputStream stream { juce::File(argv[1]) };
                require(stream.openedOk() && juce::PNGImageFormat().writeImageToStream(image, stream),
                        "write optional GUI preview");
            }

            require(performanceTab->isEnabled() && performanceTab->isVisible(),
                    "PERFORMANCE tab is available in EDIT view");
            performanceTab->onClick();
            require(performanceTab->getToggleState(), "PERFORMANCE tab becomes active");
            require(!editTab->getToggleState(), "EDIT tab deactivates in PERFORMANCE view");
            require(performancePanel->isVisible(), "PERFORMANCE tab shows its panel");
            for (auto* child : editor->getChildren())
                require(editor->getLocalBounds().contains(child->getBounds()),
                        "PERFORMANCE view remains inside each fixed preset size");
            for (auto* child : performancePanel->getChildren())
                require(performancePanel->getLocalBounds().contains(child->getBounds()),
                        "PERFORMANCE controls remain inside their section");
            const auto performanceImage = editor->createComponentSnapshot(editor->getLocalBounds());
            require(performanceImage.isValid(), "PERFORMANCE view renders at every supported size");
            editTab->onClick();
            require(editTab->getToggleState() && !performanceTab->getToggleState()
                    && !performancePanel->isVisible(), "EDIT tab restores the editor view");
        }
        std::cout << "PASS: About artwork and EDIT/PERFORMANCE views render at all five fixed GUI sizes\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
