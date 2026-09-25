#include "PluginProcessor.h"

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

int main()
{
    juce::ScopedJuceInitialiser_GUI gui;
    try
    {
        VDX7AudioProcessor processor(false);
        std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
        require(editor != nullptr, "editor creation without firmware");

        const juce::Colour accent(0xff575248);
        constexpr std::array<float, 3> decorationY { 22.0f, 34.0f, 46.0f };
        for (const int width : { 1080, 1440, 1800 })
        {
            const int height = juce::roundToInt(width * 1110.0 / 1440.0);
            editor->setSize(width, height);
            const auto image = editor->createComponentSnapshot(editor->getLocalBounds());
            require(image.isValid(), "header snapshot at supported scale");
            const float scaleX = float(width) / 1440.0f;
            const float scaleY = float(height) / 1110.0f;
            const int headerX = juce::roundToInt(720.0f * scaleX);
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
