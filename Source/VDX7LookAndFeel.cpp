#include "VDX7LookAndFeel.h"
#include "VDX7MechanicalDrawing.h"

#include <BinaryData.h>

namespace
{
juce::Image loadImage(const char* data, int size)
{
    return juce::ImageFileFormat::loadFrom(data, static_cast<std::size_t>(size));
}
}

VDX7LookAndFeel::VDX7LookAndFeel()
    : buttonNormal_(loadImage(VDX7Assets::buttonnormal_png, VDX7Assets::buttonnormal_pngSize)),
      buttonHover_(loadImage(VDX7Assets::buttonhover_png, VDX7Assets::buttonhover_pngSize)),
      buttonPressed_(loadImage(VDX7Assets::buttonpressed_png, VDX7Assets::buttonpressed_pngSize)),
      buttonActive_(loadImage(VDX7Assets::buttonactive_png, VDX7Assets::buttonactive_pngSize)),
      tabNormal_(loadImage(VDX7Assets::tabnormal_png, VDX7Assets::tabnormal_pngSize)),
      tabHover_(loadImage(VDX7Assets::tabhover_png, VDX7Assets::tabhover_pngSize)),
      tabPressed_(loadImage(VDX7Assets::tabpressed_png, VDX7Assets::tabpressed_pngSize)),
      tabActive_(loadImage(VDX7Assets::tabactive_png, VDX7Assets::tabactive_pngSize)),
      knobNormal_(loadImage(VDX7Assets::knob80normal_png, VDX7Assets::knob80normal_pngSize)),
      knobHover_(loadImage(VDX7Assets::knob80hover_png, VDX7Assets::knob80hover_pngSize)),
      knobPressed_(loadImage(VDX7Assets::knob80pressed_png, VDX7Assets::knob80pressed_pngSize)),
      knobActive_(loadImage(VDX7Assets::knob80active_png, VDX7Assets::knob80active_pngSize)),
      faderTrack_(loadImage(VDX7Assets::fadertrack_png, VDX7Assets::fadertrack_pngSize)),
      faderThumbNormal_(loadImage(VDX7Assets::faderthumbnormal_png, VDX7Assets::faderthumbnormal_pngSize)),
      faderThumbHover_(loadImage(VDX7Assets::faderthumbhover_png, VDX7Assets::faderthumbhover_pngSize)),
      faderThumbPressed_(loadImage(VDX7Assets::faderthumbpressed_png, VDX7Assets::faderthumbpressed_pngSize)),
      faderThumbActive_(loadImage(VDX7Assets::faderthumbactive_png, VDX7Assets::faderthumbactive_pngSize)),
      wheelBase_(loadImage(VDX7Assets::wheelbase_png, VDX7Assets::wheelbase_pngSize)),
      wheelMarker_(loadImage(VDX7Assets::wheelpositionmarker_png, VDX7Assets::wheelpositionmarker_pngSize))
{
    // The source fader track contains 60 px of transparent padding at both ends.
    // Keep only the visible rail so it can fill the complete slider travel.
    faderTrack_ = faderTrack_.getClippedImage(
        faderTrack_.getBounds().withTrimmedTop(60).withTrimmedBottom(60));

    setColour(juce::Label::textColourId, juce::Colour(0xffe6eef0));
    setColour(juce::ComboBox::textColourId, juce::Colour(0xffe6eef0));
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff071014));
    setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff31515a));
    setColour(juce::ComboBox::arrowColourId, juce::Colour(0xff00e7e7));
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff0b181b));
    setColour(juce::PopupMenu::textColourId, juce::Colour(0xffe6eef0));
    setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff087b82));
    setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
}

const juce::Image& VDX7LookAndFeel::buttonImage(bool isTab, bool active,
                                                bool highlighted, bool down) const
{
    if (isTab)
    {
        if (down) return tabPressed_;
        if (active) return tabActive_;
        if (highlighted) return tabHover_;
        return tabNormal_;
    }

    if (down) return buttonPressed_;
    if (active) return buttonActive_;
    if (highlighted) return buttonHover_;
    return buttonNormal_;
}

const juce::Image& VDX7LookAndFeel::knobImage(bool active, bool highlighted, bool down) const
{
    if (down) return knobPressed_;
    if (active) return knobActive_;
    if (highlighted) return knobHover_;
    return knobNormal_;
}

const juce::Image& VDX7LookAndFeel::faderThumbImage(bool active, bool highlighted,
                                                    bool down) const
{
    if (down) return faderThumbPressed_;
    if (active) return faderThumbActive_;
    if (highlighted) return faderThumbHover_;
    return faderThumbNormal_;
}

void VDX7LookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                            const juce::Colour&, bool highlighted, bool down)
{
    const bool isTab = static_cast<bool>(button.getProperties().getWithDefault("vdx7Tab", false));
    const auto& image = buttonImage(isTab, button.getToggleState(), highlighted, down);
    juce::Graphics::ScopedSaveState state(g);
    g.setOpacity(button.isEnabled() ? 1.0f : 0.35f);
    if (bool(button.getProperties().getWithDefault("vdx7WideHeader", false)))
    {
        const int inset = juce::roundToInt(image.getWidth() * 0.10f);
        g.drawImage(image, 0, 0, button.getWidth(), button.getHeight(),
                    inset, 0, image.getWidth() - 2 * inset, image.getHeight());
        return;
    }
    g.drawImage(image, button.getLocalBounds().toFloat());
}

void VDX7LookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& button,
                                      bool, bool down)
{
    const bool active = button.getToggleState();
    g.setColour(active ? juce::Colour(0xff031012) : juce::Colour(0xffe6eef0));
    g.setFont(getTextButtonFont(button, button.getHeight()));
    g.drawFittedText(button.getButtonText(), button.getLocalBounds().reduced(5).translated(0, down ? 1 : 0),
                     juce::Justification::centred, 1);
}

juce::Font VDX7LookAndFeel::getTextButtonFont(juce::TextButton&, int buttonHeight)
{
    return juce::Font(juce::FontOptions(juce::jlimit(8.0f, 18.0f, buttonHeight * 0.34f),
                                         juce::Font::bold));
}

void VDX7LookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float startAngle, float endAngle,
                                        juce::Slider& slider)
{
    const auto& image = knobNormal_;
    const float diameter = static_cast<float>(juce::jmin(width, height));
    const auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                                static_cast<float>(width), static_cast<float>(height))
                            .withSizeKeepingCentre(diameter, diameter)
                            .reduced(2.0f);
    g.drawImage(image, bounds);

    if (slider.isMouseOverOrDragging())
    {
        juce::Graphics::ScopedSaveState save(g);
        g.setOpacity(slider.isMouseButtonDown() ? 0.16f : 0.08f);
        g.drawImage(knobHover_, bounds);
    }

    const float angle = startAngle + sliderPos * (endAngle - startAngle);
    const float radius = bounds.getWidth() * 0.39f;
    juce::Path marker;
    marker.addRoundedRectangle(-1.5f, -radius, 3.0f, radius * 0.37f, 1.5f);
    marker.applyTransform(juce::AffineTransform::rotation(angle)
                              .translated(bounds.getCentreX(), bounds.getCentreY()));
    g.setColour(juce::Colour(0xff00e7e7));
    g.fillPath(marker);
}

void VDX7LookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float minSliderPos, float maxSliderPos,
                                        juce::Slider::SliderStyle style, juce::Slider& slider)
{
    const bool horizontal = bool(slider.getProperties().getWithDefault("vdx7ModeSwitch", false));
    if (horizontal || bool(slider.getProperties().getWithDefault("vdx7SyncSwitch", false)))
    {
        const auto body = slider.getLocalBounds().toFloat().withSizeKeepingCentre(
            slider.getWidth() * (horizontal ? 0.88f : 0.46f),
            slider.getHeight() * (horizontal ? 0.46f : 0.88f));
        const bool on = slider.getValue() >= 0.5;
        g.setColour(juce::Colour(0xff020709));
        g.fillRoundedRectangle(body, 3.0f);
        g.setColour(juce::Colour(0xff466069));
        g.drawRoundedRectangle(body, 3.0f, 1.0f);
        auto lever = body.reduced(3.0f);
        if (horizontal)
        {
            lever.setWidth(body.getWidth() * 0.40f);
            lever.setX(on ? body.getRight() - lever.getWidth() - 3.0f : body.getX() + 3.0f);
        }
        else
        {
            lever.setHeight(body.getHeight() * 0.40f);
            lever.setY(on ? body.getY() + 3.0f : body.getBottom() - lever.getHeight() - 3.0f);
        }
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff849498), lever.getTopLeft(),
                                              juce::Colour(0xff19262d), lever.getBottomLeft(), false));
        g.fillRoundedRectangle(lever, 2.0f);
        g.setColour(on ? juce::Colour(0xff00e7e7) : juce::Colour(0xff90a4aa));
        g.fillRect(lever.reduced(2.0f).withHeight(2.0f).withY(lever.getCentreY()));
        return;
    }
    if (style != juce::Slider::LinearVertical)
    {
        juce::LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos,
                                               minSliderPos, maxSliderPos, style, slider);
        return;
    }

    const auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                                static_cast<float>(width), static_cast<float>(height));

    const bool isFader = static_cast<bool>(
        slider.getProperties().getWithDefault("vdx7Fader", false));
    if (isFader)
    {
        const float thumbWidth = bounds.getWidth();
        const float thumbHeight = thumbWidth * (28.0f / 58.0f);
        const float trackWidth = bounds.getWidth() * (36.0f / 58.0f);
        const auto trackBounds = bounds.withSizeKeepingCentre(
            trackWidth, juce::jmax(1.0f, bounds.getHeight() - thumbHeight * 0.35f));
        g.drawImage(faderTrack_, trackBounds);

        const float thumbCentreY = juce::jlimit(bounds.getY() + thumbHeight * 0.5f,
                                                bounds.getBottom() - thumbHeight * 0.5f,
                                                sliderPos);
        const auto thumbBounds = juce::Rectangle<float>(thumbWidth, thumbHeight)
                                     .withCentre({ bounds.getCentreX(), thumbCentreY });
        const auto& thumb = faderThumbImage(slider.hasKeyboardFocus(false),
                                            slider.isMouseOverOrDragging(),
                                            slider.isMouseButtonDown());
        if (static_cast<bool>(slider.getProperties().getWithDefault("vdx7MasterFader",false)))
            VDX7MechanicalDrawing::faderCap(g,thumbBounds,slider.isMouseOverOrDragging(),slider.isMouseButtonDown());
        else g.drawImage(thumb, thumbBounds);
        return;
    }

    const auto wheelBounds = bounds;
    const float value=static_cast<float>(slider.valueToProportionOfLength(slider.getValue()));
    VDX7MechanicalDrawing::wheel(g,wheelBounds,value,slider.isMouseOverOrDragging());
}

void VDX7LookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool,
                                    int buttonX, int, int buttonW, int,
                                    juce::ComboBox& box)
{
    const auto bounds = juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(width),
                                                static_cast<float>(height)).reduced(0.5f);
    g.setColour(box.findColour(juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle(bounds, 3.0f);
    g.setColour(box.findColour(juce::ComboBox::outlineColourId));
    if (static_cast<bool>(box.getProperties().getWithDefault("vdx7LcdCombo",false)))
    {
        g.drawRect(bounds, 1.0f);
        g.drawVerticalLine(buttonX, bounds.getY(), bounds.getBottom());
    }
    else g.drawRoundedRectangle(bounds, 3.0f, 1.0f);

    juce::Path arrow;
    const float centreX = buttonX + buttonW * 0.5f;
    const float centreY = height * 0.5f;
    arrow.startNewSubPath(centreX - 4.0f, centreY - 2.0f);
    arrow.lineTo(centreX, centreY + 2.0f);
    arrow.lineTo(centreX + 4.0f, centreY - 2.0f);
    g.setColour(box.findColour(juce::ComboBox::arrowColourId));
    g.strokePath(arrow, juce::PathStrokeType(1.5f));
}

juce::Font VDX7LookAndFeel::getComboBoxFont(juce::ComboBox& box)
{
    if (static_cast<bool>(box.getProperties().getWithDefault("vdx7LcdCombo",false)))
        return juce::Font(juce::FontOptions(juce::jlimit(8.0f,18.0f,box.getHeight()*0.58f)));
    return juce::Font(juce::FontOptions(juce::jlimit(8.0f, 18.0f, box.getHeight() * 0.42f)));
}

void VDX7LookAndFeel::positionComboBoxText(juce::ComboBox& box, juce::Label& label)
{
    if (!bool(box.getProperties().getWithDefault("vdx7LcdCombo", false)))
        return juce::LookAndFeel_V4::positionComboBoxText(box, label);

    label.setBounds(1, 1, box.getWidth() - box.getHeight(), box.getHeight() - 2);
    label.setBorderSize(juce::BorderSize<int>(0, 3, 0, 2));
    label.setFont(getComboBoxFont(box));
}

void VDX7LookAndFeel::drawComboBoxTextWhenNothingSelected(
    juce::Graphics& g, juce::ComboBox& box, juce::Label& label)
{
    if (!bool(box.getProperties().getWithDefault("vdx7LcdCombo", false)))
        return juce::LookAndFeel_V4::drawComboBoxTextWhenNothingSelected(g, box, label);

    g.setColour(box.findColour(juce::ComboBox::textColourId));
    g.setFont(getComboBoxFont(box));
    g.drawFittedText(box.getTextWhenNothingSelected(),
                     label.getBorderSize().subtractedFrom(label.getBounds()),
                     label.getJustificationType(), 1);
}
