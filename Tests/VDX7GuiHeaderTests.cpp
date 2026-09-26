#include "PluginEditor.h"

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
}

int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    try
    {
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

        const juce::Colour accent(0xff71685b);
        constexpr std::array<float, 3> decorationY { 22.0f, 34.0f, 46.0f };
        for (const int width : { 1080, 1440, 1800 })
        {
            const int height = juce::roundToInt(width * 1110.0 / 1440.0);
            editor->setSize(width, height);
            const auto image = editor->createComponentSnapshot(editor->getLocalBounds());
            require(image.isValid(), "header snapshot at supported scale");
            const float scaleX = float(width) / 1440.0f;
            const float scaleY = float(height) / 1110.0f;
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
            if (argc == 2 && width == 1440)
            {
                juce::FileOutputStream stream { juce::File(argv[1]) };
                require(stream.openedOk() && juce::PNGImageFormat().writeImageToStream(image, stream),
                        "write optional GUI preview");
            }
        }
        std::cout << "PASS: three header accents match divider color at 75/100/125% sizes\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
