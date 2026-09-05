#include "VDX7VoiceData.h"
#include "VDX7Sysex.h"

#include <array>
#include <cstdlib>
#include <iostream>

namespace
{
void require(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}
}

int main()
{
    std::array<uint8_t, VDX7VoiceData::kPackedVoiceSize> voice {};

    for (int op = 0; op < VDX7VoiceData::kOperatorCount; ++op)
    {
        for (int p = 0; p < VDX7VoiceData::kParameterCount; ++p)
        {
            const auto parameter = static_cast<VDX7VoiceData::Parameter>(p);
            const int minimum = VDX7VoiceData::parameterMinimum(parameter);
            const int maximum = VDX7VoiceData::parameterMaximum(parameter);
            for (int value = minimum; value <= maximum; ++value)
            {
                require(VDX7VoiceData::setOperatorParameter(
                            voice.data(), voice.size(), op, parameter, value),
                        "parameter write");
                require(VDX7VoiceData::getOperatorParameter(
                            voice.data(), voice.size(), op, parameter) == value,
                        "parameter round trip");
            }
        }
    }

    VDX7VoiceData::setOperatorParameter(voice.data(), voice.size(), 0,
                                        VDX7VoiceData::Parameter::detune, 99);
    require(VDX7VoiceData::getOperatorParameter(
                voice.data(), voice.size(), 0, VDX7VoiceData::Parameter::detune) == 7,
            "upper range clamp");
    VDX7VoiceData::setOperatorParameter(voice.data(), voice.size(), 0,
                                        VDX7VoiceData::Parameter::outputLevel, -1);
    require(VDX7VoiceData::getOperatorParameter(
                voice.data(), voice.size(), 0, VDX7VoiceData::Parameter::outputLevel) == 0,
            "lower range clamp");

    // OP1 occupies the last packed operator block, while OP6 occupies the first.
    VDX7VoiceData::setOperatorParameter(voice.data(), voice.size(), 0,
                                        VDX7VoiceData::Parameter::outputLevel, 91);
    VDX7VoiceData::setOperatorParameter(voice.data(), voice.size(), 5,
                                        VDX7VoiceData::Parameter::outputLevel, 27);
    require(voice[5 * 17 + 14] == 91, "OP1 VMEM ordering");
    require(voice[14] == 27, "OP6 VMEM ordering");

    // Editing one packed bit-field must preserve its neighbours.
    voice[5 * 17 + 12] = 0x80;
    VDX7VoiceData::setOperatorParameter(voice.data(), voice.size(), 0,
                                        VDX7VoiceData::Parameter::rateScaling, 6);
    VDX7VoiceData::setOperatorParameter(voice.data(), voice.size(), 0,
                                        VDX7VoiceData::Parameter::detune, -3);
    require((voice[5 * 17 + 12] & 0x80) != 0, "reserved bit preservation");
    require(VDX7VoiceData::getOperatorParameter(
                voice.data(), voice.size(), 0, VDX7VoiceData::Parameter::rateScaling) == 6,
            "rate-scaling preservation");

    voice[5 * 17 + 11] = 0xf0;
    VDX7VoiceData::setOperatorParameter(voice.data(), voice.size(), 0,
                                        VDX7VoiceData::Parameter::leftScaleCurve, 1);
    VDX7VoiceData::setOperatorParameter(voice.data(), voice.size(), 0,
                                        VDX7VoiceData::Parameter::rightScaleCurve, 2);
    require(voice[5 * 17 + 11] == 0xf9, "scaling-curve bit packing");

    voice[5 * 17 + 15] = 0xc0;
    VDX7VoiceData::setOperatorParameter(voice.data(), voice.size(), 0,
                                        VDX7VoiceData::Parameter::oscillatorMode, 1);
    VDX7VoiceData::setOperatorParameter(voice.data(), voice.size(), 0,
                                        VDX7VoiceData::Parameter::coarse, 23);
    require(voice[5 * 17 + 15] == 0xef, "mode/coarse bit packing");

    for (int p = 0; p < VDX7VoiceData::kVoiceParameterCount; ++p)
    {
        const auto parameter = static_cast<VDX7VoiceData::VoiceParameter>(p);
        const int minimum = VDX7VoiceData::voiceParameterMinimum(parameter);
        const int maximum = VDX7VoiceData::voiceParameterMaximum(parameter);
        for (int value = minimum; value <= maximum; ++value)
        {
            require(VDX7VoiceData::setVoiceParameter(
                        voice.data(), voice.size(), parameter, value),
                    "voice parameter write");
            require(VDX7VoiceData::getVoiceParameter(
                        voice.data(), voice.size(), parameter) == value,
                    "voice parameter round trip");
        }
    }

    voice[111] = 0xf0;
    VDX7VoiceData::setVoiceParameter(voice.data(), voice.size(),
                                     VDX7VoiceData::VoiceParameter::feedback, 6);
    VDX7VoiceData::setVoiceParameter(voice.data(), voice.size(),
                                     VDX7VoiceData::VoiceParameter::oscillatorKeySync, 1);
    require(voice[111] == 0xfe, "feedback/key-sync bit packing");

    voice[116] = 0x80;
    VDX7VoiceData::setVoiceParameter(voice.data(), voice.size(),
                                     VDX7VoiceData::VoiceParameter::lfoKeySync, 1);
    VDX7VoiceData::setVoiceParameter(voice.data(), voice.size(),
                                     VDX7VoiceData::VoiceParameter::lfoWaveform, 4);
    VDX7VoiceData::setVoiceParameter(voice.data(), voice.size(),
                                     VDX7VoiceData::VoiceParameter::pitchModSensitivity, 5);
    require(voice[116] == 0xd9, "LFO packed bit preservation");

    // Canonical synthetic voice: no user ROM or factory patch in test fixtures.
    std::vector<uint8_t> synthetic(128, 0);
    for (int op=0;op<6;++op)
        for (int f=0;f<VDX7VoiceData::kParameterCount;++f)
        {
            auto p=static_cast<VDX7VoiceData::Parameter>(f);
            VDX7VoiceData::setOperatorParameter(synthetic.data(),128,op,p,
                VDX7VoiceData::parameterMaximum(p));
        }
    for (int f=0;f<VDX7VoiceData::kVoiceParameterCount;++f)
    {
        auto p=static_cast<VDX7VoiceData::VoiceParameter>(f);
        VDX7VoiceData::setVoiceParameter(synthetic.data(),128,p,
            VDX7VoiceData::voiceParameterMaximum(p));
    }
    for (int i=118;i<128;++i) synthetic[i]='A'+i-118;
    auto single=VDX7Sysex::encode(synthetic);
    require(single.size()==163 && single[3]==0 && single[4]==1 && single[5]==27,"VCED header");
    require(single[6+20]==14 && single[6+17]==1 && single[6+18]==31,"VCED detune/mode/coarse positions");
    require(single[140]==31 && single[150]==48 && single[151]=='A',"VCED algorithm/transpose/name positions");
    std::vector<uint8_t> decoded;
    for (int device=0;device<16;++device)
    {
        single[2]=static_cast<uint8_t>(device);
        require(VDX7Sysex::decode(single,decoded) && decoded==synthetic,"VCED round trip all devices");
    }
    auto broken=single;
    broken[6]^=1;
    require(!VDX7Sysex::decode(broken,decoded) && decoded==synthetic,"bad checksum is nonmutating");
    broken=single; broken[6]|=128;
    require(!VDX7Sysex::decode(broken,decoded),"reject non-7-bit data");
    broken=single; broken.pop_back();
    require(!VDX7Sysex::decode(broken,decoded),"reject truncated message");
    broken=single; broken[2]=16;
    require(!VDX7Sysex::decode(broken,decoded),"reject invalid device");
    broken=single; broken[6]=100;
    unsigned sum=0; for (int i=6;i<161;++i) sum+=broken[i];
    broken[161]=static_cast<uint8_t>((128-(sum&127))&127);
    require(!VDX7Sysex::decode(broken,decoded),"reject out-of-range VCED field even with valid checksum");
    std::vector<uint8_t> bank(4096,0);
    for (int i=0;i<32;++i) std::copy(synthetic.begin(),synthetic.end(),bank.begin()+i*128);
    auto bankMessage=VDX7Sysex::encode(bank);
    require(bankMessage.size()==4104 && bankMessage[3]==9,"VMEM header");
    require(VDX7Sysex::decode(bankMessage,decoded) && decoded==bank,"VMEM full bank round trip");
    require(!VDX7Sysex::decode({},decoded) && VDX7Sysex::encode({}).empty(),"reject empty input");
    std::cout << "VDX7 voice-data and SysEx tests passed\n";
    return 0;
}
