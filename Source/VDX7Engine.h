#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "dx7.h"
#include "VDX7VoiceData.h"
#include "VDX7Resampler.h"

class VDX7Engine
{
public:
    static constexpr double kNativeSampleRate = VDX7Resampler::nativeRate;
    static constexpr std::size_t kFirmwareSize = 16384;
    static constexpr std::size_t kFactoryVoicesSize = 32768;
    static constexpr std::size_t kCombinedRomSize = kFirmwareSize + kFactoryVoicesSize;
    static constexpr std::size_t kRamStateSize = 6144;

    VDX7Engine();
    ~VDX7Engine() = default;

    bool loadRomImage(const uint8_t* data, std::size_t size,
                      const uint8_t* optionalVoices = nullptr,
                      std::size_t optionalVoicesSize = 0);

    bool isLoaded() const noexcept { return loaded_; }
    bool hasFactoryVoices() const noexcept { return factoryVoices_.size() >= kFactoryVoicesSize; }

    void prepare(double hostSampleRate);
    int latencySamples() const noexcept { return resampler_.latency(); }
    void resetAudioState();
    void resetMidiLifecycle(); // Non-RT; host has stopped processBlock.
    // Engine-owner-only host reset. No allocation or extra warm-up render.
    // The processor mutes output and defers incoming MIDI while this drains.
    void beginHostReset();
    void advanceHostReset(int sampleBudget);
    bool isHostResetInProgress() const noexcept { return hostResetInProgress_; }
    void render(float* left, float* right, int numSamples);

    void handleMidi(const uint8_t* data, int size);
    bool handleSysex(const uint8_t* data, std::size_t size);
    void allNotesOff(bool useRunningStatus = false);
    bool hasHeldMidiNotes() const noexcept;
    uint64_t midiOverloadCount() const noexcept { return midiOverloadCount_; }
    bool isMidiRecovering() const noexcept { return midiRecovering_; }

    bool loadSyxBank(const uint8_t* data, std::size_t size);
    bool selectFactoryBank(int bankIndex);
    // Engine-thread token: advances only when factory voice RAM is replaced,
    // including reselecting the same bank. Not part of saved project state.
    uint64_t factoryBankLoadRevision() const noexcept { return factoryBankLoadRevision_; }
    void selectProgram(int programIndex);

    int currentBank() const noexcept { return currentBank_; }
    void setCurrentBankMarker(int bankIndex) noexcept { currentBank_ = (bankIndex >= 0 && bankIndex <= 7) ? bankIndex : -1; }
    int currentProgram() const noexcept { return currentProgram_; }
    std::string currentProgramName() const;
    void copyCurrentProgramName(char* destination, std::size_t capacity) const noexcept;
    int getOperatorParameter(int operatorIndex, VDX7VoiceData::Parameter) const noexcept;
    bool setOperatorParameter(int operatorIndex, VDX7VoiceData::Parameter, int value) noexcept;
    int getVoiceParameter(VDX7VoiceData::VoiceParameter) const noexcept;
    bool setVoiceParameter(VDX7VoiceData::VoiceParameter, int value) noexcept;
    void reloadCurrentProgram();

    bool saveRam(std::vector<uint8_t>& out) const;
    bool restoreRam(const std::vector<uint8_t>& in);

    // Global battery-RAM controller settings, NOT voice/SysEx parameters.
    // Controller order: wheel, foot, breath, aftertouch. Field 0: range 0-99;
    // fields 1-3: pitch, amplitude, EG-bias assignments (0/1).
    int getControllerSetting(int controller, int field) const noexcept;
    int getPitchBendSetting(int field) const noexcept;
    int getPlaySetting(int field) const noexcept;
    int masterTune() const noexcept;
    bool setMasterTune(int value) noexcept;
    // UI-only: a play-mode change drains a bounded firmware reset and ends notes.
    bool setPlaySetting(int field, int value);
    bool setPitchBendSetting(int field, int value) noexcept;
    bool setControllerSetting(int controller, int field, int value) noexcept;

private:
    uint64_t factoryBankLoadRevision_ = 0;
    friend struct VDX7RegressionAccess;
    void boot();
    void processQueuedMessage(dx7Emu::Message msg);
    void parseMidiBytes(const uint8_t* data, int size);
    bool reserveMidi(int bytes);
    void recoverMidiOverflow();
    int generateNative(float* out);
    float nextNativeSample();
    uint8_t mapVelocity(uint8_t velocity) const;
    uint8_t* currentPackedVoice() noexcept;
    const uint8_t* currentPackedVoice() const noexcept;

    dx7Emu::ToSynth* toSynth_ = nullptr;
    dx7Emu::ToGui* toGui_ = nullptr;
    dx7Emu::App_ToSynth appToSynth_;
    dx7Emu::NullToGui nullToGui_;
    dx7Emu::DX7 dx7_;

    std::vector<uint8_t> factoryVoices_;
    // Stable, blank fallback for the core's non-owning cartridge pointer.
    std::array<uint8_t, 4096> emptyFactoryBank_ {};

    static constexpr int kNativeBlockSize = 512;
    std::array<float, kNativeBlockSize> nativeBlock_{};
    int nativePos_ = 0;
    int nativeCount_ = 0;

    double hostSampleRate_ = 48000.0;
    VDX7Resampler resampler_;

    float volume_ = 1.0f;
    float midiExpression_ = 1.0f;
    std::array<uint8_t, 128> velocityMap_{};
    // Firmware has 16 voices and releases one matching voice per note-off.
    static constexpr uint8_t kMaxRepeatedNotes = 16;
    std::array<uint8_t, 128> activeMidiNotes_{};
    // Conservative release budget: queued offs may be flushed before firmware
    // consumes them. Keep peak multiplicity (bounded at 16) until a verified
    // normal-playback idle boundary; unknown ROMs retain it since ROM load.
    std::array<uint8_t, 128> midiReleaseBudget_{};
    // Only the locally validated v1.8 image has a known dispatch/RAM profile.
    bool releaseRetirementProfile_ = false;
    bool releaseHistoryDirty_ = false;
    static bool isReleaseRetirementFirmware(const uint8_t* data, std::size_t size);
    void retireCompletedReleaseHistory();
    bool sustainDown_ = false;
    bool midiRecovering_ = false;
    uint64_t midiOverloadCount_ = 0;
    unsigned controllerRefreshMessages_ = 0;
    bool pitchBendRefresh_ = false;
    bool portamentoRefresh_ = false;
    uint8_t lastPitchBendInput_ = 64;

    bool hostResetInProgress_ = false;
    bool hostResetMuted_ = false; // Runtime-only; cleared by an accepted fresh Note On.
    bool loaded_ = false;
    int currentBank_ = -1;
    int currentProgram_ = 0;
};
