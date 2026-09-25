#include "VDX7AboutPanel.h"
#include <BinaryData.h>

VDX7AboutPanel::VDX7AboutPanel()
    : wordmark_(juce::Drawable::createFromImageData(VDX7Assets::vdx7silver2_svg, VDX7Assets::vdx7silver2_svgSize)),
      gyr_(juce::Drawable::createFromImageData(VDX7Assets::gyrvector_svg, VDX7Assets::gyrvector_svgSize)),
      signature_(juce::Drawable::createFromImageData(VDX7Assets::developersignature_svg, VDX7Assets::developersignature_svgSize)),
      source_("github.com/RobCZart82/VDX7-JUCE", juce::URL("https://github.com/RobCZart82/VDX7-JUCE"))
{
    setName("About VDX7 Mk 1.");
    setLookAndFeel(&lookAndFeel_);
    for (auto* label : { &subtitle_, &developerCaption_, &version_, &licenses_, &firmware_ })
    {
        label->setJustificationType(juce::Justification::centred);
        label->setColour(juce::Label::textColourId, juce::Colour(0xffeee9dc));
        label->setFont(juce::Font(juce::FontOptions(16.0f)));
        addAndMakeVisible(*label);
    }
    subtitle_.setText("6-Operator FM Synthesizer / Hardware Emulation", juce::dontSendNotification);
    developerCaption_.setText("Developed by", juce::dontSendNotification);
    developerCaption_.setColour(juce::Label::textColourId, juce::Colour(0xffbdb8ac));
    if (signature_)
        signature_->replaceColour(juce::Colour(0xff211a19), juce::Colour(0xffeee9dc));
    version_.setText("Version " VDX7_DISPLAY_VERSION, juce::dontSendNotification);
    licenses_.setText("VDX7-JUCE: GNU AGPLv3, without warranty.\n"
        "DX7 core: chiaccona / Retromulator, GPLv3-or-later.\n"
        "JUCE: AGPLv3. Source and license notices at the link above.", juce::dontSendNotification);
    licenses_.setFont(juce::Font(juce::FontOptions(14.0f)));
    firmware_.setText("No Yamaha logo or firmware is included.\n"
        "A user-supplied compatible ROM is required.", juce::dontSendNotification);
    firmware_.setFont(juce::Font(juce::FontOptions(14.0f)));
    firmware_.setColour(juce::Label::textColourId, juce::Colour(0xffc9d68e));
    source_.setColour(juce::HyperlinkButton::textColourId, juce::Colour(0xff68c7bb));
    source_.setName("VDX7 source and licenses");
    source_.setFont(juce::Font(juce::FontOptions(17.0f)), false);
    addAndMakeVisible(source_);
    addAndMakeVisible(close_);
    close_.addShortcut(juce::KeyPress(juce::KeyPress::escapeKey));
    close_.addShortcut(juce::KeyPress(juce::KeyPress::returnKey));
    close_.onClick = [this]
    {
        if (auto* window = findParentComponentOfClass<juce::DialogWindow>())
            window->exitModalState(0);
    };
    setSize(650, 590);
}

VDX7AboutPanel::~VDX7AboutPanel() { setLookAndFeel(nullptr); }

void VDX7AboutPanel::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff403a32), bounds.getTopLeft(),
        juce::Colour(0xff24221e), bounds.getBottomRight(), false));
    g.fillAll();
    g.setColour(juce::Colour(0xff71685b));
    g.drawRect(bounds.reduced(0.5f), 1.0f);
    if (wordmark_) wordmark_->drawWithin(g, { 60, 38, 530, 70 }, juce::RectanglePlacement::centred, 1.0f);
    if (gyr_) gyr_->drawWithin(g, { 64, 200, 110, 158 }, juce::RectanglePlacement::centred, 1.0f);
    // Keep the original developer-name slot; enlarge the mark within its
    // existing vertical gap without changing the About layout.
    if (signature_) signature_->drawWithin(g, { 130, 240, 390, 54 }, juce::RectanglePlacement::centred, 1.0f);
    g.setColour(juce::Colour(0xff68c7bb).withAlpha(0.65f));
    for (int y : { 175, 398 }) g.drawHorizontalLine(y, 40.0f, 610.0f);
}

void VDX7AboutPanel::resized()
{
    subtitle_.setBounds(35, 126, 580, 28);
    developerCaption_.setBounds((getWidth() - 290) / 2, 209, 290, 24);
    version_.setBounds((getWidth() - 310) / 2, 310, 310, 26);
    source_.setBounds(70, 362, 510, 26);
    licenses_.setBounds(35, 411, 580, 60);
    firmware_.setBounds(35, 480, 580, 40);
    close_.setBounds(245, 539, 160, 36);
}
