#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <vector>

#include "VDX7Engine.h"

namespace VDX7ParameterIDs
{
inline constexpr auto masterVolume = "masterVolume";
inline constexpr auto pitchWheel = "pitchWheel";
inline constexpr auto modWheel = "modWheel";
juce::String operatorParameter(int operatorIndex, VDX7VoiceData::Parameter);
juce::String voiceParameter(VDX7VoiceData::VoiceParameter);
}

class VDX7AudioProcessor final : public juce::AudioProcessor,
                                  private juce::AudioProcessorValueTreeState::Listener
{
public:
    VDX7AudioProcessor();
    ~VDX7AudioProcessor() override;

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
    bool exportSyx(const juce::File&, bool entireBank, juce::String& error);
    bool renameVoice(const juce::String& name);
    bool copyOperator(int op);
    bool pasteOperator(int op);
    bool hasCopiedOperator() const noexcept { return hasOperatorClipboard_.load(); }
    bool hasUnexportedEdits() const noexcept;
    bool isCurrentVoiceModified() const noexcept;

    bool isRomLoaded() const;
    bool hasFactoryVoices() const;
    int getCurrentBank() const;
    juce::String getCurrentPatchName() const;
    juce::String getRomPath() const;
    juce::String getStatusText() const;

    juce::File getSuggestedRomFolder() const;

    juce::AudioProcessorValueTreeState& parameters() noexcept { return parameters_; }
    juce::MidiKeyboardState& keyboardState() noexcept { return keyboardState_; }
    float getOutputPeak(int channel) const noexcept;
    double getCpuUsagePercent() const;
    uint32_t getOperatorVoiceRevision() const noexcept;
    bool synchroniseOperatorParametersFromEngine();

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    bool autoDetectRom();
    static bool readFile(const juce::File& file, std::vector<uint8_t>& data);
    bool loadRomData(const juce::File& file, const std::vector<uint8_t>& rom, juce::String* error);
    static juce::String bankName(int index);
    void applyPendingCommands();
    bool applyOperatorParameters();
    bool applyVoiceParameters();
    void applyPerformanceControls();
    void updateEngineSnapshot() noexcept;
    void synchroniseVoiceParametersLocked();
    void clearMeters() noexcept;
    void updateMeters(const juce::AudioBuffer<float>& buffer) noexcept;
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void markVoiceModified() noexcept;
    void flushVoiceEditsLocked();

    mutable std::mutex engineMutex_;
    mutable std::mutex metadataMutex_;
    VDX7Engine engine_;
    juce::AudioProcessorValueTreeState parameters_;
    juce::MidiKeyboardState keyboardState_;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> outputGain_;
    juce::AudioProcessLoadMeasurer processLoadMeasurer_;

    std::atomic<float>* masterVolumeParameter_ = nullptr;
    std::atomic<float>* pitchWheelParameter_ = nullptr;
    std::atomic<float>* modWheelParameter_ = nullptr;
    std::array<std::array<juce::String, VDX7VoiceData::kParameterCount>,
               VDX7VoiceData::kOperatorCount> operatorParameterIDs_;
    std::array<std::array<std::atomic<float>*, VDX7VoiceData::kParameterCount>,
               VDX7VoiceData::kOperatorCount> operatorParameterValues_ {};
    std::array<std::atomic<uint64_t>, 2> operatorParameterDirty_ {};
    std::array<std::array<std::atomic<float>, VDX7VoiceData::kParameterCount>,
               VDX7VoiceData::kOperatorCount> pendingOperatorValues_ {};
    std::array<juce::String, VDX7VoiceData::kVoiceParameterCount> voiceParameterIDs_;
    std::array<std::atomic<float>*, VDX7VoiceData::kVoiceParameterCount>
        voiceParameterValues_ {};
    std::atomic<uint32_t> voiceParameterDirty_ { 0 };
    std::array<std::atomic<float>, VDX7VoiceData::kVoiceParameterCount> pendingVoiceValues_ {};
    std::atomic<uint32_t> operatorVoiceRevision_ { 0 };
    // Tracks voices changed since import/export, not the DAW's project-save state.
    std::atomic<uint32_t> modifiedVoices_ { 0 };
    std::array<uint8_t, 17> operatorClipboard_ {};
    std::atomic<bool> hasOperatorClipboard_ { false };

    std::atomic<bool> engineLoaded_ { false };
    std::atomic<bool> factoryVoicesAvailable_ { false };
    std::atomic<int> currentBankSnapshot_ { -1 };
    std::atomic<int> currentProgramSnapshot_ { 0 };
    std::atomic<uint32_t> patchNameRevision_ { 0 };
    std::array<std::atomic<char>, 11> patchNameSnapshot_ {};
    std::atomic<float> outputPeakLeft_ { 0.0f };
    std::atomic<float> outputPeakRight_ { 0.0f };

    // Single-value command mailboxes keep routine UI changes off the engine
    // mutex. The audio thread consumes the newest request at block boundaries.
    std::atomic<int> pendingBank_ { -1 };
    std::atomic<int> pendingProgram_ { -1 };
    int lastPitchMsb_ = -1;
    int lastModValue_ = -1;

    double currentSampleRate_ = 48000.0;
    juce::File romFile_;
    juce::String statusText_ { "ROM not loaded" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VDX7AudioProcessor)
};
