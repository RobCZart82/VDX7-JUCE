// Private-ROM comparison tool: binary float stream on stdout, not a CTest.
#include <VDX7Engine.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <vector>

int main(int argc, char** argv)
{
    if (argc != 2) return 2;
    std::ifstream file(argv[1], std::ios::binary);
    std::vector<uint8_t> rom((std::istreambuf_iterator<char>(file)), {});
    if (rom.size() != 16384) return 3;
    for (int rate : {44100, 48000, 96000})
    for (int algorithm : {0, 15, 31})
    for (int feedback : {0, 7})
    {
        VDX7Engine e;
        if (!e.loadRomImage(rom.data(), rom.size())) return 4;
        e.prepare(rate);
        using P = VDX7VoiceData::Parameter;
        using V = VDX7VoiceData::VoiceParameter;
        for (int op = 0; op < 6; ++op)
        {
            for (int field = 0; field < VDX7VoiceData::kParameterCount; ++field)
                e.setOperatorParameter(op, static_cast<P>(field), 0);
            for (const auto p : {P::rate1, P::rate2, P::rate3, P::rate4}) e.setOperatorParameter(op, p, 80);
            for (const auto p : {P::level1, P::level2, P::level3}) e.setOperatorParameter(op, p, 99);
            e.setOperatorParameter(op, P::outputLevel, 75);
            e.setOperatorParameter(op, P::coarse, op + 1);
        }
        for (int field = 0; field < VDX7VoiceData::kVoiceParameterCount; ++field)
            e.setVoiceParameter(static_cast<V>(field), 0);
        for (const auto p : {V::pitchLevel1, V::pitchLevel2, V::pitchLevel3, V::pitchLevel4}) e.setVoiceParameter(p, 50);
        e.setVoiceParameter(V::algorithm, algorithm);
        e.setVoiceParameter(V::feedback, feedback);
        e.setVoiceParameter(V::transpose, 24);
        e.reloadCurrentProgram();
        const uint8_t on[] {0x90, 60, 100}, off[] {0x80, 60, 0};
        e.handleMidi(on, 3);
        std::vector<float> out(32768);
        for (int position = 0; position < 32768;)
        {
            if (position == 16384) e.handleMidi(off, 3);
            const int count = std::min({257, 32768 - position, position < 16384 ? 16384 - position : 32768 - position});
            e.render(out.data() + position, nullptr, count);
            position += count;
        }
        std::cout.write(reinterpret_cast<const char*>(out.data()), std::streamsize(out.size() * sizeof(float)));
    }
    return 0;
}
