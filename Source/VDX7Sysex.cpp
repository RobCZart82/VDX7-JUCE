#include "VDX7Sysex.h"
#include "VDX7VoiceData.h"
#include <algorithm>
#include <array>

namespace VDX7Sysex
{
using P = VDX7VoiceData::Parameter;
using V = VDX7VoiceData::VoiceParameter;
// Yamaha VCED order, each block OP6 through OP1 (not host parameter order).
constexpr std::array<P, 21> fields {
    P::rate1,P::rate2,P::rate3,P::rate4,P::level1,P::level2,P::level3,P::level4,
    P::breakpoint,P::leftScaleDepth,P::rightScaleDepth,P::leftScaleCurve,
    P::rightScaleCurve,P::rateScaling,P::amplitudeModSensitivity,
    P::velocitySensitivity,P::outputLevel,P::oscillatorMode,P::coarse,P::fine,P::detune
};

bool decode(const std::vector<uint8_t>& m, std::vector<uint8_t>& packed)
{
    const bool single = m.size() == kVoiceMessageSize;
    if ((!single && m.size() != kBankMessageSize) || m[0] != 0xf0 || m[1] != 0x43
        || m[2] > 15 || m.back() != 0xf7
        || m[3] != (single ? 0 : 9) || m[4] != (single ? 1 : 32)
        || m[5] != (single ? 27 : 0)) return false;
    if (std::any_of(m.begin()+6, m.end()-1, [](uint8_t v){ return v > 127; }))
        return false;
    unsigned sum = 0;
    for (auto i = m.begin()+6; i != m.end()-1; ++i) sum += *i;
    if ((sum & 127) != 0) return false;
    std::vector<uint8_t> result(single ? 128 : 4096, 0);
    if (!single)
    {
        std::copy(m.begin()+6, m.end()-2, result.begin());

        // A checksum-valid VMEM bank can still contain an invalid packed
        // detune nibble (15 encodes +8, outside the DX7 range -7..+7).
        for (int voice = 0; voice < 32; ++voice)
        {
            const auto* packedVoice = result.data() + voice * VDX7VoiceData::kPackedVoiceSize;
            if (!VDX7VoiceData::hasValidPackedVoice(
                    packedVoice, VDX7VoiceData::kPackedVoiceSize))
                return false;
        }
    }
    else
    {
        for (int block = 0; block < 6; ++block)
            for (int f = 0; f < 21; ++f)
            {
                auto p = fields[f];
                int v = m[6 + block*21 + f] - (p == P::detune ? 7 : 0);
                if (v < VDX7VoiceData::parameterMinimum(p)
                    || v > VDX7VoiceData::parameterMaximum(p)) return false;
                VDX7VoiceData::setOperatorParameter(result.data(),128,5-block,p,v);
            }
        for (int f = 0; f < 19; ++f)
        {
            auto p = static_cast<V>(f);
            int v = m[132+f] + (p == V::algorithm ? 1 : p == V::transpose ? -24 : 0);
            if (v < VDX7VoiceData::voiceParameterMinimum(p)
                || v > VDX7VoiceData::voiceParameterMaximum(p)) return false;
            VDX7VoiceData::setVoiceParameter(result.data(),128,p,v);
        }
        std::copy(m.begin()+151,m.begin()+161,result.begin()+118);
    }
    packed = std::move(result);
    return true;
}

std::vector<uint8_t> encode(const std::vector<uint8_t>& packed)
{
    const bool single = packed.size() == 128;
    if (!single && packed.size() != 4096) return {};
    const int voiceCount = single ? 1 : 32;
    for (int voice = 0; voice < voiceCount; ++voice)
        if (!VDX7VoiceData::hasValidPackedVoice(
                packed.data() + voice * VDX7VoiceData::kPackedVoiceSize,
                VDX7VoiceData::kPackedVoiceSize))
            return {};
    std::vector<uint8_t> m(single ? kVoiceMessageSize : kBankMessageSize,0);
    m[0]=0xf0; m[1]=0x43; m[3]=single ? 0 : 9;
    m[4]=single ? 1 : 32; m[5]=single ? 27 : 0;
    if (!single) std::copy(packed.begin(),packed.end(),m.begin()+6);
    else
    {
        for (int block=0;block<6;++block)
            for (int f=0;f<21;++f)
                m[6+block*21+f]=static_cast<uint8_t>(VDX7VoiceData::getOperatorParameter(
                    packed.data(),128,5-block,fields[f]) + (fields[f]==P::detune ? 7 : 0));
        for (int f=0;f<19;++f)
        {
            auto p=static_cast<V>(f);
            m[132+f]=static_cast<uint8_t>(VDX7VoiceData::getVoiceParameter(packed.data(),128,p)
                + (p==V::algorithm ? -1 : p==V::transpose ? 24 : 0));
        }
        std::copy(packed.begin()+118,packed.end(),m.begin()+151);
    }
    if (std::any_of(m.begin()+6,m.end()-2,[](uint8_t v){return v>127;})) return {};
    unsigned sum=0;
    for (auto i=m.begin()+6;i!=m.end()-2;++i) sum+=*i;
    m[m.size()-2]=static_cast<uint8_t>((128-(sum&127))&127);
    m.back()=0xf7;
    return m;
}
}
