#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <cstdint>
#include <mutex>
#include "VDX7LatestDisplay.h"
#include <vector>

#include "VDX7Engine.h"
#include "VDX7DeferredMidi.h"
#include "VDX7EditQueue.h"
#include "VDX7KeyboardQueue.h"
#include "VDX7UserBank.h"

namespace VDX7ParameterIDs
{
inline constexpr auto masterVolume = "masterVolume";
inline constexpr auto pitchWheel = "pitchWheel";
inline constexpr auto modWheel = "modWheel";
juce::String operatorParameter(int operatorIndex, VDX7VoiceData::Parameter);
juce::String voiceParameter(VDX7VoiceData::VoiceParameter);
}

class VDX7AudioProcessor final : public juce::AudioProcessor,
                                  private juce::AudioProcessorValueTreeState::Listener,
                                  private juce::MidiKeyboardState::Listener,
                                  private juce::Timer
{
public:
    explicit VDX7AudioProcessor(bool detectRom = true);
    ~VDX7AudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void reset() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

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
    // Message-thread library operations. Capture is immutable across open dialogs.
    bool captureUserPatch(VDX7UserBank::Voice&, juce::String& error);
    bool loadUserBank(const juce::File&, juce::String& error);
    static juce::File userBankFile();
    bool renameVoice(const juce::String& name);
    bool copyOperator(int op);
    bool pasteOperator(int op);
    bool hasCopiedOperator() const noexcept { return hasOperatorClipboard_.load(); }
    bool hasUnexportedEdits() const noexcept;
    bool isCurrentVoiceModified() const noexcept;
    // Message-thread controls, saved in existing RAM state; not host automation.
    // Legacy-shaped read accessors decode the coherent, lock-free display
    // snapshot. They are safe for ordinary periodic editor refreshes.
    std::array<int, 16> getControllerSettings() const;
    struct PerformanceDisplay { std::array<int,16> controllers{}; std::array<int,4> play{}; std::array<int,2> bend{}; };
    PerformanceDisplay getPerformanceDisplay() const noexcept;
    std::array<int, 2> getPitchBendSettings() const;
    std::array<int, 4> getPlaySettings() const;
    int getMasterTune() const;
    int getMidiInputChannel() const noexcept { return midiInputChannel_.load(); }
    bool setMidiInputChannelFromUi(int channel);
    struct MonoCorrectionStatus { bool requested, active, loaded; };
    MonoCorrectionStatus getMonoCorrectionStatus() const;
    // Non-RT, explicit user action: changing mode releases all playing notes.
    bool setMonoCorrectionFromUi(bool enabled);
    bool setMasterTuneFromUi(int value);
    bool setPlaySettingFromUi(int field, int value);
    bool setPitchBendSettingFromUi(int field, int value);
    bool setControllerSettingFromUi(int controller, int field, int value);

    bool isRomLoaded() const;
    bool hasFactoryVoices() const;
    int getCurrentBank() const;
    juce::String getCurrentPatchName() const;
    juce::String getRomPath() const;
    juce::String getStatusText() const;

    juce::File getSuggestedRomFolder() const;

    juce::AudioProcessorValueTreeState& parameters() noexcept { return parameters_; }
    // UI/message-thread interface. Audio communicates only through the queue
    // and atomic snapshots, never by calling MidiKeyboardState.
    juce::MidiKeyboardState& keyboardState() noexcept { return keyboardState_; }
    float getOutputPeak(int channel) const noexcept;
    double getCpuUsagePercent() const;
    uint32_t getOperatorVoiceRevision() const noexcept;
    // Non-realtime only: captures under engine lock, notifies after unlocking.
    bool synchroniseOperatorParametersFromEngine();

private:
    bool loadPackedVoices(const std::vector<uint8_t>&, juce::String* error, int selectProgram = -1);
    friend struct VDX7RegressionAccess;
    void restoreSavedStateLocked(const juce::ValueTree&);
    bool observeStateInstall(); // Audio-thread owned; also rechecked under engine lock.
    void discardStaleCollectedInput(juce::MidiBuffer&, std::size_t& keyboardCount);
    void capturePendingRestoreEditsLocked();
    juce::ValueTree pendingRestore_;
    bool detectRom_ = true;
    // The host may request a reset from a processing thread. Only publish a
    // request here; processBlock owns the MIDI queues and the engine cleanup.
    static_assert(std::atomic<bool>::is_always_lock_free);
    std::atomic<bool> hostResetRequested_ {false};
    bool hostResetPending_ = false; // Audio-thread owned across contention.
    // State restoration is initiated off the audio thread. The audio callback
    // observes this epoch before touching its own deferred MIDI storage.
    std::atomic<uint64_t> midiTimelineEpoch_ {0};
    uint64_t audioMidiTimelineEpoch_ = 0; // Audio-thread owned.
    bool stateRestoreReleasePending_ = false; // Audio-thread owned across contention.
    VDX7DeferredMidi deferredMidi_;
    bool handleMidiEventLocked(const uint8_t*, int);
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    bool autoDetectRom();
    static bool readFile(const juce::File& file, std::size_t maxBytes,
                         std::vector<uint8_t>& data);
    bool loadRomData(const juce::File& file, const std::vector<uint8_t>& rom, juce::String* error);
    static juce::String bankName(int index);
    void applyPendingCommands();
    bool applyOperatorParameters();
    bool applyVoiceParameters();
    // Applies coalesced global settings on the engine/audio thread. POLY/MONO
    // retains its firmware reset transaction; portamento time is a bounded
    // three-byte serial update and is safe to commit here.
    void applyPendingPerformanceSettings() noexcept;
    void applyPerformanceControls();
    void updateEngineSnapshot() noexcept;
    void publishPerformanceDisplay() noexcept; // Caller owns engineMutex_.
    void publishMasterTune() noexcept; // Caller owns engineMutex_.
    uint64_t capturePerformanceDisplay() const noexcept;
    uint64_t captureMasterTuneDisplay() const noexcept;
    void setPerformanceDisplayBits(uint64_t mask, uint64_t value) noexcept;
    static_assert(std::atomic<uint64_t>::is_always_lock_free);
    VDX7LatestDisplay performanceDisplay_;
    // The UI can coalesce frequent global edits without taking engineMutex_.
    // Bits 0-15: controller fields, 16-18: play fields 1-3, 19-20: bend,
    // bit 21: master tuning. The audio thread owns their firmware application.
    static constexpr uint32_t kMasterTunePerformanceMask = uint32_t {1} << 21;
    std::atomic<uint32_t> pendingPerformanceDirty_ {0};
    std::array<std::atomic<int>, 16> pendingControllerSettings_ {};
    std::array<std::atomic<int>, 3> pendingPlaySettings_ {};
    std::array<std::atomic<int>, 2> pendingPitchBendSettings_ {};
    std::atomic<int> pendingMasterTune_ {0};
    VDX7LatestDisplay masterTuneSnapshot_ {256}; // Nine-bit offset encoding: -256..255.
    void timerCallback() override;
    void handleNoteOn(juce::MidiKeyboardState*, int, int, float) override;
    void handleNoteOff(juce::MidiKeyboardState*, int, int, float) override;
    void mirrorKeyboardOnMessageThread();
    void clearKeyboardSnapshot() noexcept;
    VDX7KeyboardQueue keyboardQueue_;
    static_assert(std::atomic<uint16_t>::is_always_lock_free);
    std::array<std::atomic<uint16_t>, 128> keyboardSnapshot_ {};
    std::array<std::atomic<uint16_t>, 128> keyboardUiHeld_ {};
    std::atomic<bool> voicePublicationNeeded_ {false};
    std::atomic<bool> publishingParameters_ {false};
    void clearMeters() noexcept;
    void updateMeters(const juce::AudioBuffer<float>& buffer) noexcept;
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void markVoiceModified() noexcept;
    void flushVoiceEditsLocked();

    mutable std::mutex engineMutex_;
    // Instance-lifetime diagnostics; never persisted or reset by the GUI.
    std::atomic<uint64_t> contendedAudioBlocks_ {0}, contendedAudioSamples_ {0};
    std::atomic<uint64_t> longestContendedAudioRunSamples_ {0};
    std::atomic<uint64_t> lastModeTransactionMicros_ {0}, peakModeTransactionMicros_ {0};
    uint64_t audioContendedRunSamples_ = 0; // Audio-thread owned.
    std::atomic<int> midiInputChannel_ {0}; // 0=legacy OMNI, 1-16=host input filter.
    int audioMidiInputChannel_ = 0; // Audio-thread owned.
    bool channelReleasePending_ = false; // Audio-thread owned across contention.
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
    std::atomic<uint64_t> midiOverloadSnapshot_ {0};
    std::atomic<bool> factoryVoicesAvailable_ { false };
    std::atomic<int> currentBankSnapshot_ { -1 };
    std::atomic<int> currentProgramSnapshot_ { 0 };
    std::atomic<uint32_t> patchNameRevision_ { 0 };
    std::array<std::atomic<char>, 11> patchNameSnapshot_ {};
    std::atomic<float> outputPeakLeft_ { 0.0f };
    std::atomic<float> outputPeakRight_ { 0.0f };

    VDX7EditQueue editQueue_;
    int lastPitchMsb_ = -1;
    int lastModValue_ = -1;

    double currentSampleRate_ = 48000.0;
    juce::File romFile_;
    juce::String statusText_ { "ROM not loaded" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VDX7AudioProcessor)
};
