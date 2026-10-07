#pragma once
#include "VDX7VoiceData.h"
#include <algorithm>
#include <array>

// VDX7-owned neutral editing seed, not a dump of Yamaha's INIT voice.
// Produces data only; applying it to a working voice is a separate transaction.
inline std::array<uint8_t, VDX7VoiceData::kPackedVoiceSize> vdx7InitVoice()
{
    using P = VDX7VoiceData::Parameter;
    using V = VDX7VoiceData::VoiceParameter;
    std::array<uint8_t, VDX7VoiceData::kPackedVoiceSize> voice {};
    for (int op = 0; op < VDX7VoiceData::kOperatorCount; ++op)
    {
        const auto set = [&](P p, int value) { VDX7VoiceData::setOperatorParameter(voice.data(), voice.size(), op, p, value); };
        for (P p : {P::rate1, P::rate2, P::rate3}) set(p, 99);
        set(P::rate4, 80);
        for (P p : {P::level1, P::level2, P::level3}) set(p, 99);
        set(P::level4, 0);
        set(P::outputLevel, op == 0 ? 99 : 0);
        set(P::coarse, 1);
        set(P::detune, 0);
    }
    const auto set = [&](V p, int value) { VDX7VoiceData::setVoiceParameter(voice.data(), voice.size(), p, value); };
    set(V::algorithm, 32);
    set(V::transpose, 0);
    for (V p : {V::pitchRate1, V::pitchRate2, V::pitchRate3, V::pitchRate4}) set(p, 99);
    for (V p : {V::pitchLevel1, V::pitchLevel2, V::pitchLevel3, V::pitchLevel4}) set(p, 50);
    std::fill(voice.begin() + 118, voice.end(), uint8_t(' '));
    constexpr char name[] = "Init Preset";
    std::copy_n(name, 10, voice.begin() + 118);
    return voice;
}
