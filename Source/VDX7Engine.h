#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "dx7.h"

class VDX7Engine
{
public:
    static constexpr double kNativeSampleRate = 49096.0;
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
    void resetAudioState();
    void render(float* left, float* right, int numSamples);

    void handleMidi(const uint8_t* data, int size);
    void handleSysex(const uint8_t* data, std::size_t size);
    void allNotesOff();

    bool loadSyxBank(const uint8_t* data, std::size_t size);
    bool selectFactoryBank(int bankIndex);
    void selectProgram(int programIndex);

    int currentBank() const noexcept { return currentBank_; }
    void setCurrentBankMarker(int bankIndex) noexcept { currentBank_ = (bankIndex >= 0 && bankIndex <= 7) ? bankIndex : -1; }
    int currentProgram() const noexcept { return currentProgram_; }
    std::string currentProgramName() const;

    bool saveRam(std::vector<uint8_t>& out) const;
    bool restoreRam(const std::vector<uint8_t>& in);

private:
    void boot();
    void processQueuedMessage(dx7Emu::Message msg);
    void parseMidiBytes(const uint8_t* data, int size);
    int generateNative(float* out, int maxSamples);
    float nextNativeSample();
    uint8_t mapVelocity(uint8_t velocity) const;

    dx7Emu::ToSynth* toSynth_ = nullptr;
    dx7Emu::ToGui* toGui_ = nullptr;
    dx7Emu::App_ToSynth appToSynth_;
    dx7Emu::NullToGui nullToGui_;
    dx7Emu::DX7 dx7_;

    std::vector<uint8_t> factoryVoices_;

    static constexpr int kNativeBlockSize = 512;
    std::array<float, kNativeBlockSize> nativeBlock_{};
    std::array<float, kNativeBlockSize> discardBlock_{};
    int nativePos_ = 0;
    int nativeCount_ = 0;

    double hostSampleRate_ = 48000.0;
    double cpuCyclesPerNativeSample_ = 0.0;
    double cpuCycleBudget_ = 0.0;
    double resamplePhase_ = 0.0;
    float resampleA_ = 0.0f;
    float resampleB_ = 0.0f;
    bool resamplerPrimed_ = false;

    float volume_ = 1.0f;
    float midiExpression_ = 0.0f;
    std::array<uint8_t, 128> velocityMap_{};

    bool loaded_ = false;
    int currentBank_ = -1;
    int currentProgram_ = 0;
};
