#include "VDX7VoiceData.h"

#include <algorithm>

namespace VDX7VoiceData
{
namespace
{
int operatorOffset(int operatorIndex) noexcept
{
    // DX7 VMEM stores the operators in the physical OP6, OP5, ... OP1 order.
    return (kOperatorCount - 1 - operatorIndex) * kPackedOperatorSize;
}
}

int parameterMinimum(Parameter parameter) noexcept
{
    return parameter == Parameter::detune ? -7 : 0;
}

int parameterMaximum(Parameter parameter) noexcept
{
    switch (parameter)
    {
        case Parameter::coarse:                  return 31;
        case Parameter::detune:                  return 7;
        case Parameter::rateScaling:             return 7;
        case Parameter::velocitySensitivity:     return 7;
        case Parameter::amplitudeModSensitivity: return 3;
        case Parameter::oscillatorMode:           return 1;
        case Parameter::leftScaleCurve:           return 3;
        case Parameter::rightScaleCurve:          return 3;
        case Parameter::count:                   return 0;
        default:                                 return 99;
    }
}

int getOperatorParameter(const uint8_t* packedVoice, std::size_t size,
                         int operatorIndex, Parameter parameter) noexcept
{
    if (packedVoice == nullptr || size < kPackedVoiceSize
        || operatorIndex < 0 || operatorIndex >= kOperatorCount)
        return 0;

    const auto* op = packedVoice + operatorOffset(operatorIndex);
    const int index = static_cast<int>(parameter);

    if (index >= static_cast<int>(Parameter::rate1)
        && index <= static_cast<int>(Parameter::level4))
        return std::min<int>(op[index], 99);

    switch (parameter)
    {
        case Parameter::outputLevel:             return std::min<int>(op[14], 99);
        case Parameter::coarse:                  return (op[15] >> 1) & 0x1f;
        case Parameter::fine:                    return std::min<int>(op[16], 99);
        case Parameter::detune:                  return static_cast<int>((op[12] >> 3) & 0x0f) - 7;
        case Parameter::rateScaling:             return op[12] & 0x07;
        case Parameter::velocitySensitivity:     return (op[13] >> 2) & 0x07;
        case Parameter::amplitudeModSensitivity: return op[13] & 0x03;
        case Parameter::oscillatorMode:           return op[15] & 0x01;
        case Parameter::breakpoint:               return std::min<int>(op[8], 99);
        case Parameter::leftScaleDepth:           return std::min<int>(op[9], 99);
        case Parameter::rightScaleDepth:          return std::min<int>(op[10], 99);
        case Parameter::leftScaleCurve:           return op[11] & 0x03;
        case Parameter::rightScaleCurve:          return (op[11] >> 2) & 0x03;
        case Parameter::count:                   break;
        default:                                 break;
    }

    return 0;
}

bool setOperatorParameter(uint8_t* packedVoice, std::size_t size,
                          int operatorIndex, Parameter parameter, int value) noexcept
{
    if (packedVoice == nullptr || size < kPackedVoiceSize
        || operatorIndex < 0 || operatorIndex >= kOperatorCount
        || parameter == Parameter::count)
        return false;

    value = std::clamp(value, parameterMinimum(parameter), parameterMaximum(parameter));
    auto* op = packedVoice + operatorOffset(operatorIndex);
    const int index = static_cast<int>(parameter);

    if (index >= static_cast<int>(Parameter::rate1)
        && index <= static_cast<int>(Parameter::level4))
    {
        op[index] = static_cast<uint8_t>(value);
        return true;
    }

    switch (parameter)
    {
        case Parameter::outputLevel:
            op[14] = static_cast<uint8_t>(value);
            return true;
        case Parameter::coarse:
            op[15] = static_cast<uint8_t>((op[15] & 0xc1) | ((value & 0x1f) << 1));
            return true;
        case Parameter::fine:
            op[16] = static_cast<uint8_t>(value);
            return true;
        case Parameter::detune:
            op[12] = static_cast<uint8_t>((op[12] & 0x87) | (((value + 7) & 0x0f) << 3));
            return true;
        case Parameter::rateScaling:
            op[12] = static_cast<uint8_t>((op[12] & 0xf8) | (value & 0x07));
            return true;
        case Parameter::velocitySensitivity:
            op[13] = static_cast<uint8_t>((op[13] & 0xe3) | ((value & 0x07) << 2));
            return true;
        case Parameter::amplitudeModSensitivity:
            op[13] = static_cast<uint8_t>((op[13] & 0xfc) | (value & 0x03));
            return true;
        case Parameter::oscillatorMode:
            op[15] = static_cast<uint8_t>((op[15] & 0xfe) | (value & 0x01));
            return true;
        case Parameter::breakpoint:
            op[8] = static_cast<uint8_t>(value);
            return true;
        case Parameter::leftScaleDepth:
            op[9] = static_cast<uint8_t>(value);
            return true;
        case Parameter::rightScaleDepth:
            op[10] = static_cast<uint8_t>(value);
            return true;
        case Parameter::leftScaleCurve:
            op[11] = static_cast<uint8_t>((op[11] & 0xfc) | (value & 0x03));
            return true;
        case Parameter::rightScaleCurve:
            op[11] = static_cast<uint8_t>((op[11] & 0xf3) | ((value & 0x03) << 2));
            return true;
        case Parameter::count:
            break;
        default:
            break;
    }

    return false;
}

int voiceParameterMinimum(VoiceParameter parameter) noexcept
{
    return parameter == VoiceParameter::transpose ? -24
         : parameter == VoiceParameter::algorithm ? 1 : 0;
}

int voiceParameterMaximum(VoiceParameter parameter) noexcept
{
    switch (parameter)
    {
        case VoiceParameter::algorithm:             return 32;
        case VoiceParameter::feedback:              return 7;
        case VoiceParameter::oscillatorKeySync:     return 1;
        case VoiceParameter::lfoKeySync:            return 1;
        case VoiceParameter::lfoWaveform:           return 5;
        case VoiceParameter::pitchModSensitivity:   return 7;
        case VoiceParameter::transpose:             return 24;
        case VoiceParameter::count:                 return 0;
        default:                                    return 99;
    }
}

int getVoiceParameter(const uint8_t* packedVoice, std::size_t size,
                      VoiceParameter parameter) noexcept
{
    if (packedVoice == nullptr || size < kPackedVoiceSize)
        return 0;

    const int index = static_cast<int>(parameter);
    if (index >= static_cast<int>(VoiceParameter::pitchRate1)
        && index <= static_cast<int>(VoiceParameter::pitchLevel4))
        return std::min<int>(packedVoice[102 + index], 99);

    switch (parameter)
    {
        case VoiceParameter::algorithm:             return (packedVoice[110] & 0x1f) + 1;
        case VoiceParameter::feedback:              return packedVoice[111] & 0x07;
        case VoiceParameter::oscillatorKeySync:     return (packedVoice[111] >> 3) & 0x01;
        case VoiceParameter::lfoSpeed:               return std::min<int>(packedVoice[112], 99);
        case VoiceParameter::lfoDelay:               return std::min<int>(packedVoice[113], 99);
        case VoiceParameter::pitchModDepth:          return std::min<int>(packedVoice[114], 99);
        case VoiceParameter::amplitudeModDepth:      return std::min<int>(packedVoice[115], 99);
        case VoiceParameter::lfoKeySync:             return packedVoice[116] & 0x01;
        case VoiceParameter::lfoWaveform:            return std::min<int>((packedVoice[116] >> 1) & 0x07, 5);
        case VoiceParameter::pitchModSensitivity:   return (packedVoice[116] >> 4) & 0x07;
        case VoiceParameter::transpose:             return std::min<int>(packedVoice[117], 48) - 24;
        case VoiceParameter::count:                 break;
        default:                                    break;
    }

    return 0;
}

bool setVoiceParameter(uint8_t* packedVoice, std::size_t size,
                       VoiceParameter parameter, int value) noexcept
{
    if (packedVoice == nullptr || size < kPackedVoiceSize
        || parameter == VoiceParameter::count)
        return false;

    value = std::clamp(value, voiceParameterMinimum(parameter),
                       voiceParameterMaximum(parameter));
    const int index = static_cast<int>(parameter);
    if (index >= static_cast<int>(VoiceParameter::pitchRate1)
        && index <= static_cast<int>(VoiceParameter::pitchLevel4))
    {
        packedVoice[102 + index] = static_cast<uint8_t>(value);
        return true;
    }

    switch (parameter)
    {
        case VoiceParameter::algorithm:
            packedVoice[110] = static_cast<uint8_t>((packedVoice[110] & 0xe0)
                                                    | ((value - 1) & 0x1f));
            return true;
        case VoiceParameter::feedback:
            packedVoice[111] = static_cast<uint8_t>((packedVoice[111] & 0xf8)
                                                    | (value & 0x07));
            return true;
        case VoiceParameter::oscillatorKeySync:
            packedVoice[111] = static_cast<uint8_t>((packedVoice[111] & 0xf7)
                                                    | ((value & 0x01) << 3));
            return true;
        case VoiceParameter::lfoSpeed:
            packedVoice[112] = static_cast<uint8_t>(value);
            return true;
        case VoiceParameter::lfoDelay:
            packedVoice[113] = static_cast<uint8_t>(value);
            return true;
        case VoiceParameter::pitchModDepth:
            packedVoice[114] = static_cast<uint8_t>(value);
            return true;
        case VoiceParameter::amplitudeModDepth:
            packedVoice[115] = static_cast<uint8_t>(value);
            return true;
        case VoiceParameter::lfoKeySync:
            packedVoice[116] = static_cast<uint8_t>((packedVoice[116] & 0xfe)
                                                    | (value & 0x01));
            return true;
        case VoiceParameter::lfoWaveform:
            packedVoice[116] = static_cast<uint8_t>((packedVoice[116] & 0xf1)
                                                    | ((value & 0x07) << 1));
            return true;
        case VoiceParameter::pitchModSensitivity:
            packedVoice[116] = static_cast<uint8_t>((packedVoice[116] & 0x8f)
                                                    | ((value & 0x07) << 4));
            return true;
        case VoiceParameter::transpose:
            packedVoice[117] = static_cast<uint8_t>(value + 24);
            return true;
        case VoiceParameter::count:
            break;
        default:
            break;
    }

    return false;
}
}
