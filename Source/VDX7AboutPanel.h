#pragma once
#include <JuceHeader.h>
#include "VDX7LookAndFeel.h"

// Owns its rendering resources independently of the plugin editor's lifetime.
class VDX7AboutPanel final : public juce::Component
{
public:
    VDX7AboutPanel();
    ~VDX7AboutPanel() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    bool hasVectorLogos() const { return wordmark_ != nullptr && gyr_ != nullptr; }

private:
    VDX7LookAndFeel lookAndFeel_;
    std::unique_ptr<juce::Drawable> wordmark_, gyr_;
    juce::Label subtitle_, developerCaption_, developer_, version_, licenses_, firmware_;
    juce::HyperlinkButton source_;
    juce::TextButton close_ { "OK" };
};
