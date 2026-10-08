#pragma once
#include <JuceHeader.h>
#include "VDX7ImportedBanks.h"

class VDX7ImportedBankMenuItem final : public juce::PopupMenu::CustomComponent,
                                     public juce::SettableTooltipClient
{
public:
    static constexpr int menuWidth = 260;
    static constexpr int menuHeight = 26;
    static juce::String label(const VDX7ImportedBanks::Bank& bank, int index)
    {
        const auto prefix = juce::String(index + 1) + ". ";
        const int remaining = VDX7ImportedBanks::maxDisplayCharacters - prefix.length();
        auto name = bank.displayName;
        if (name.length() > remaining)
            name = name.substring(0, remaining - 1) + juce::String::charToString(0x2026);
        return prefix + name;
    }
    static juce::String details(const VDX7ImportedBanks::Bank& bank)
    { return bank.fileName + "\nBank ID: " + bank.contentId; }

    VDX7ImportedBankMenuItem(const VDX7ImportedBanks::Bank& bank, int index)
        : juce::PopupMenu::CustomComponent(true), text_(label(bank, index))
    {
        setName(bank.fileName);
        setTooltip(details(bank));
    }
    void getIdealSize(int& width, int& height) override
    { width = menuWidth; height = menuHeight; }
    void paint(juce::Graphics& g) override
    {
        const auto* item = getItem();
        const bool highlighted = isItemHighlighted();
        g.fillAll(findColour(highlighted ? juce::PopupMenu::highlightedBackgroundColourId
                                       : juce::PopupMenu::backgroundColourId));
        g.setColour(findColour(highlighted ? juce::PopupMenu::highlightedTextColourId
                                          : juce::PopupMenu::textColourId)
                        .withMultipliedAlpha(item != nullptr && !item->isEnabled ? 0.5f : 1.0f));
        g.setFont(juce::Font(juce::FontOptions(14.0f)));
        if (item != nullptr && item->isTicked)
            g.drawText(juce::String::charToString(0x2713), 4, 0, 20, getHeight(), juce::Justification::centred);
        g.drawText(text_, getLocalBounds().withTrimmedLeft(28).withTrimmedRight(8),
                   juce::Justification::centredLeft, true);
    }
private:
    juce::String text_;
};
