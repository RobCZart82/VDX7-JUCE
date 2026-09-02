#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class VDX7AudioProcessorEditor final : public juce::AudioProcessorEditor,
                                        private juce::Timer
{
public:
    explicit VDX7AudioProcessorEditor(VDX7AudioProcessor&);
    ~VDX7AudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void refresh();
    void chooseRom();
    void chooseSyx();
    void showError(const juce::String& title, const juce::String& message);

    VDX7AudioProcessor& processor_;

    juce::Label title_;
    juce::Label subtitle_;
    juce::Label status_;
    juce::Label romPath_;
    juce::Label patch_;
    juce::Label bankCaption_;
    juce::Label programCaption_;

    juce::TextButton loadRom_ { "Load ROM..." };
    juce::TextButton loadSyx_ { "Load .SYX..." };
    juce::TextButton previous_ { "<" };
    juce::TextButton next_ { ">" };
    juce::ComboBox bank_;
    juce::ComboBox program_;

    std::unique_ptr<juce::FileChooser> chooser_;
    bool internalUiUpdate_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VDX7AudioProcessorEditor)
};
