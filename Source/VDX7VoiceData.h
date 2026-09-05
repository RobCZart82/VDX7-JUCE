#pragma once

#include <cstddef>
#include <cstdint>

namespace VDX7VoiceData
{
constexpr int kOperatorCount = 6;
constexpr int kPackedOperatorSize = 17;
constexpr std::size_t kPackedVoiceSize = 128;

enum class Parameter
{
    rate1,
    rate2,
    rate3,
    rate4,
    level1,
    level2,
    level3,
    level4,
    outputLevel,
    coarse,
    fine,
    detune,
    rateScaling,
    velocitySensitivity,
    amplitudeModSensitivity,
    oscillatorMode,
    breakpoint,
    leftScaleDepth,
    rightScaleDepth,
    leftScaleCurve,
    rightScaleCurve,
    count
};

constexpr int kParameterCount = static_cast<int>(Parameter::count);

enum class VoiceParameter
{
    pitchRate1,
    pitchRate2,
    pitchRate3,
    pitchRate4,
    pitchLevel1,
    pitchLevel2,
    pitchLevel3,
    pitchLevel4,
    algorithm,
    feedback,
    oscillatorKeySync,
    lfoSpeed,
    lfoDelay,
    pitchModDepth,
    amplitudeModDepth,
    lfoKeySync,
    lfoWaveform,
    pitchModSensitivity,
    transpose,
    count
};

constexpr int kVoiceParameterCount = static_cast<int>(VoiceParameter::count);

int parameterMinimum(Parameter) noexcept;
int parameterMaximum(Parameter) noexcept;
int getOperatorParameter(const uint8_t* packedVoice, std::size_t size,
                         int operatorIndex, Parameter) noexcept;
bool setOperatorParameter(uint8_t* packedVoice, std::size_t size,
                          int operatorIndex, Parameter, int value) noexcept;
int voiceParameterMinimum(VoiceParameter) noexcept;
int voiceParameterMaximum(VoiceParameter) noexcept;
int getVoiceParameter(const uint8_t* packedVoice, std::size_t size,
                      VoiceParameter) noexcept;
bool setVoiceParameter(uint8_t* packedVoice, std::size_t size,
                       VoiceParameter, int value) noexcept;
}
