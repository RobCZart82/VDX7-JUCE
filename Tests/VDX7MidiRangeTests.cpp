#include "VDX7Engine.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <cmath>
#include <algorithm>

struct VDX7RegressionAccess
{
    static int portaRate(const VDX7Engine& e) { return e.dx7_.memory[0xe0]; }
};

int main(int argc, char** argv)
{
    if (argc != 2) { std::cerr << "Supply a local combined ROM path\n"; return 77; }
    std::ifstream f(argv[1], std::ios::binary);
    std::vector<uint8_t> rom((std::istreambuf_iterator<char>(f)), {});
    int failures = 0;
    for (int note = 0; note < 128; ++note)
    {
        VDX7Engine e;
        if (!e.loadRomImage(rom.data(), rom.size())) return 77;
        e.prepare(44100);
        float l[256], r[256];
        const bool sustain = note % 5 == 0;
        uint8_t pedal[]{0xb0,64,127};
        if (sustain) e.handleMidi(pedal,3);
        uint8_t on[]{static_cast<uint8_t>(0x90 | (note % 16)), static_cast<uint8_t>(note), 100};
        e.handleMidi(on, 3);
        float peak=0;
        for (int b=0;b<100;++b) {
            e.render(l,r,256);
            for (float v:l) { if (!std::isfinite(v)) return 1; peak=std::max(peak,std::abs(v)); }
        }
        uint8_t off[]{static_cast<uint8_t>((note % 2 ? 0x90 : 0x80) | (note % 16)), static_cast<uint8_t>(note), 0};
        if (note % 3 == 0) e.allNotesOff(); else e.handleMidi(off,3);
        if (sustain) {
            for (int b=0;b<20;++b) e.render(l,r,256);
            if(note%2) { pedal[2]=0; e.handleMidi(pedal,3); }
            else e.allNotesOff(); // Also release notes already held only by sustain.
        }
        float tail = 0;
        for (int b=0;b<520;++b) {
            e.render(l,r,256);
            for (float v:l) {
                if (!std::isfinite(v)) return 1;
                if (b >= 500) tail = std::max(tail,std::abs(v));
            }
        }
        if (peak < 0.00001f || tail > 0.001f) {
            ++failures;
            std::cerr << "FAIL " << note << " peak " << peak << " tail " << tail << '\n';
        }
        if (note % 16 == 15) std::cout << "Checked through note " << note << std::endl;
    }
    // Verify pitch, not just non-silence: isolated ratio-1 carrier, no LFO/PEG.
    for (int note : {0, 24, 35, 36, 60, 96, 97, 108, 127})
    {
        VDX7Engine e;
        e.loadRomImage(rom.data(),rom.size()); e.prepare(44100);
        using P=VDX7VoiceData::Parameter; using V=VDX7VoiceData::VoiceParameter;
        e.setVoiceParameter(V::algorithm,31); e.setVoiceParameter(V::feedback,0);
        e.setVoiceParameter(V::transpose,0); e.setVoiceParameter(V::pitchModDepth,0);
        e.setVoiceParameter(V::amplitudeModDepth,0);
        for(auto p:{V::pitchLevel1,V::pitchLevel2,V::pitchLevel3,V::pitchLevel4}) e.setVoiceParameter(p,50);
        for(int op=0;op<6;++op) {
            e.setOperatorParameter(op,P::outputLevel,op==0?99:0);
            e.setOperatorParameter(op,P::oscillatorMode,0); e.setOperatorParameter(op,P::coarse,1);
            e.setOperatorParameter(op,P::fine,0); e.setOperatorParameter(op,P::detune,0);
            e.setOperatorParameter(op,P::leftScaleDepth,0); e.setOperatorParameter(op,P::rightScaleDepth,0);
            for(auto p:{P::rate1,P::rate2,P::rate3,P::rate4}) e.setOperatorParameter(op,p,99);
            for(auto p:{P::level1,P::level2,P::level3}) e.setOperatorParameter(op,p,99);
            e.setOperatorParameter(op,P::level4,0);
        }
        e.reloadCurrentProgram(); float l[256],r[256];
        for(int b=0;b<20;++b) e.render(l,r,256);
        uint8_t on[]{0x90,static_cast<uint8_t>(note),100}; e.handleMidi(on,3);
        for(int b=0;b<100;++b) e.render(l,r,256);
        int crossings=0; float prev=0;
        for(int b=0;b<344;++b) {e.render(l,r,256); for(float v:l){if(prev<0 && v>=0) ++crossings; prev=v;}}
        const double hz=crossings*44100.0/(344*256);
        const double expected=440*std::pow(2.0,(note-69)/12.0);
        std::cout << "Pitch " << note << ": " << hz << " expected " << expected << std::endl;
        if(std::abs(hz-expected)>std::max(1.0,expected*0.025)) ++failures;
        if (note == 60)
        {
            double tuningHz[3] {};
            int tuningIndex = 0;
            for (int tune : {-256, 0, 255})
            {
                if (!e.setMasterTune(tune)) ++failures;
                for (int b=0;b<100;++b) e.render(l,r,256);
                int count = 0; float last = 0;
                for (int b=0;b<344;++b)
                { e.render(l,r,256); for (float v:l) { if (last<0 && v>=0) ++count; last=v; } }
                tuningHz[tuningIndex++] = count*44100.0/(344*256);
            }
            std::cout << "Master tuning low/zero/high Hz: " << tuningHz[0] << "/"
                      << tuningHz[1] << "/" << tuningHz[2] << std::endl;
            if (!(tuningHz[0] < tuningHz[1] && tuningHz[1] < tuningHz[2])
                || std::abs(tuningHz[1]-expected)>1.0) ++failures;
            e.setMasterTune(0);
            int previousRate = 256;
            for (int time = 0; time <= 99; ++time)
            {
                if (!e.setPlaySetting(3, time)) ++failures;
                for (int b = 0; b < 4; ++b) e.render(l,r,256);
                const int rate = VDX7RegressionAccess::portaRate(e);
                if (e.getPlaySetting(3) != time || rate > previousRate || rate < 1) ++failures;
                previousRate = rate;
            }
            std::vector<uint8_t> slowState;
            e.saveRam(slowState);
            e.setPlaySetting(3, 0);
            for (int b=0;b<8;++b) e.render(l,r,256);
            if (VDX7RegressionAccess::portaRate(e) != 255) ++failures;
            e.restoreRam(slowState);
            for (int b=0;b<8;++b) e.render(l,r,256);
            if (VDX7RegressionAccess::portaRate(e) != 1) ++failures;
            e.setPlaySetting(3, 0);
            e.handleMidi(on,3); // RAM restore/program activation ended the earlier note.
            for(int b=0;b<100;++b) e.render(l,r,256);
            // Held note: each setting change must update without a new note-on.
            struct Bend { int range, step, wheel; double semitones; };
            for (const auto test : {Bend{0,0,127,0}, Bend{6,0,127,6}, Bend{12,0,127,12}, Bend{6,0,-1,6},
                                   Bend{12,0,0,-12}, Bend{0,3,127,12}, Bend{12,0,64,0}})
            {
                e.setPitchBendSetting(0, test.range); e.setPitchBendSetting(1, test.step);
                uint8_t wheel[] {0xe0,0,static_cast<uint8_t>(test.wheel)};
                if (test.wheel >= 0) e.handleMidi(wheel,3);
                for (int b=0;b<40;++b) e.render(l,r,256);
                crossings=0; prev=0;
                for(int b=0;b<172;++b) {e.render(l,r,256); for(float v:l){if(prev<0 && v>=0) ++crossings; prev=v;}}
                const double measured=crossings*44100.0/(172*256);
                const double wanted=expected*std::pow(2.0,test.semitones/12.0);
                std::cout << "Bend " << test.range << "/" << test.step << "/" << test.wheel
                          << ": " << measured << " expected " << wanted << std::endl;
                if(std::abs(measured-wanted)>std::max(1.5,wanted*0.03)) ++failures;
            }
            if (!e.setPlaySetting(0,1) || !e.setPlaySetting(1,1) || !e.setPlaySetting(2,0)) ++failures;
            uint8_t pedal[] {0xb0,65,127}, low[] {0x90,60,100}, high[] {0x90,72,100};
            e.handleMidi(pedal,3); e.handleMidi(low,3);
            for(int b=0;b<100;++b) e.render(l,r,256);
            e.setPlaySetting(3,99); e.handleMidi(high,3);
            for(int b=0;b<10;++b) e.render(l,r,256);
            auto measure = [&]
            {
                int count=0; float last=0;
                for(int b=0;b<64;++b) {e.render(l,r,256); for(float v:l){if(last<0 && v>=0) ++count; last=v;}}
                return count*44100.0/(64*256);
            };
            const double slow = measure();
            e.setPlaySetting(3,0);
            for(int b=0;b<20;++b) e.render(l,r,256);
            const double fast = measure();
            std::cout << "Mono portamento slow " << slow << " fast " << fast << std::endl;
            if (!(slow > 240 && slow < 450 && std::abs(fast-expected*2) < 12)) ++failures;
            e.allNotesOff();
            for(int b=0;b<500;++b) e.render(l,r,256);
            float tail=0; for(float v:l) tail=std::max(tail,std::abs(v));
            if(tail>0.001f || !e.setPlaySetting(0,0)) ++failures;
            for (int n=0;n<60;++n) e.selectProgram(n % 32);
            const bool accepted = e.setPlaySetting(0,1);
            for (int b=0;b<4000;++b) e.render(l,r,256);
            // A busy rejection must never turn into a delayed mode change.
            if (e.getPlaySetting(0) != (accepted ? 1 : 0)) ++failures;
            if (!e.setPlaySetting(0,1)) ++failures;
        }
    }
    return failures == 0 ? 0 : 1;
}
