#pragma once

#include <JuceHeader.h>
#include <mutex>
#include <vector>

#include "VDX7Engine.h"

class VDX7AudioProcessor final : public juce::AudioProcessor
{
public:
    VDX7AudioProcessor();
    ~VDX7AudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 32; }
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    bool loadRomFromFile(const juce::File& file, juce::String* error = nullptr);
    bool loadSyxFromFile(const juce::File& file, juce::String* error = nullptr);
    bool selectFactoryBank(int bank);
    void selectProgramFromUi(int program);

    bool isRomLoaded() const;
    bool hasFactoryVoices() const;
    int getCurrentBank() const;
    juce::String getCurrentPatchName() const;
    juce::String getRomPath() const;
    juce::String getStatusText() const;

    juce::File getSuggestedRomFolder() const;

private:
    bool autoDetectRom();
    static bool readFile(const juce::File& file, std::vector<uint8_t>& data);
    bool loadRomData(const juce::File& file, const std::vector<uint8_t>& rom, juce::String* error);
    static juce::String bankName(int index);

    mutable std::mutex engineMutex_;
    VDX7Engine engine_;
    double currentSampleRate_ = 48000.0;
    juce::File romFile_;
    juce::String statusText_ { "ROM not loaded" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VDX7AudioProcessor)
};
