#include "VDX7Engine.h"
#include <array>
#include <bit>
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>

static void require(bool ok, const char* message)
{ if (!ok) throw std::runtime_error(message); }

struct ScanProbe
{
    int phase = 0, clocks = 0, switches = 0;
    bool cleanMode = false, partialSwitch = false;
    void clean(bool mode) noexcept
    { partialSwitch |= phase != 0; cleanMode = mode; ++switches; }
    void clock(float* out, int& count, int cycles) noexcept
    {
        clocks += cycles;
        for (int i = 0; i < cycles; ++i)
            if (++phase == 96) { phase = 0; out[count++] = cleanMode ? 2.0f : 1.0f; }
    }
};

static void ordering(bool negative)
{
    VDX7NativeSoundMode adapter;
    ScanProbe egs;
    std::array<float, 600> out {};
    int count = 0;
    adapter.clock(egs, out.data(), count, 17);
    adapter.request(true);
    if (negative) egs.clean(true); // Deliberately broken mid-scan caller.
    adapter.clock(egs, out.data(), count, 79);
    require(count == 1 && out[0] == 1.0f && !egs.partialSwitch,
        "request cannot relabel the unfinished old scan");
    adapter.clock(egs, out.data(), count, 256 * 96 + 37);
    require(count == 257 && out[255] == 1.0f / 256 && out[256] == 0
        && egs.switches == 1 && !egs.partialSwitch && egs.clocks == 257 * 96 + 37,
        "exact muted full-scan switch retains instruction overshoot");
    adapter.clock(egs, out.data(), count, 256 * 96 - 37);
    require(count == 513 && out[512] == 2.0f && adapter.snapshot().level == 256,
        "Clean reaches unity after bounded fade in");
    for (int i = 1; i <= 512; ++i)
        require(out[std::size_t(i)] == (i <= 256 ? float(256 - i) / 256
            : 2.0f * float(i - 256) / 256), "gain is attached to each generated scan exactly once");
    adapter.request(false);
    adapter.clock(egs, out.data(), count, 73);
    const auto incomplete = adapter.snapshot();
    adapter.request(true);
    adapter.clock(egs, out.data(), count, 23 + 96);
    require(egs.switches == 1 && adapter.snapshot().activeClean
        && incomplete.phase == 73 && adapter.snapshot().level == 256,
        "cancellation does not change gain halfway through an existing scan");
}

static void quiescentOrdering()
{
    for (int phase = 0; phase < 96; ++phase)
    {
        VDX7NativeSoundMode adapter;
        ScanProbe egs;
        std::array<float, 8> out {};
        int count = 0;
        adapter.clock(egs, out.data(), count, phase);
        adapter.installWhileQuiescent(true);
        const auto pending = adapter.snapshot();
        require(pending.quiescentInstall && pending.phase == phase && pending.level == 256,
            "cold install preserves real scan phase and arms unity without clocks");
        adapter.clock(egs, out.data(), count, 96 - phase);
        if (phase != 0)
        {
            require(count == 0 && egs.switches == 0 && !egs.partialSwitch,
                "cold install discards only interrupted old scan, without mid-scan switch");
            adapter.clock(egs, out.data(), count, 96);
        }
        require(count == 1 && out[0] == 2 && egs.switches == 1 && !egs.partialSwitch
            && !adapter.snapshot().quiescentInstall && adapter.snapshot().level == 256,
            "first published complete cold scan is Clean at unity at every phase");
    }
    VDX7NativeSoundMode adapter;
    ScanProbe egs;
    std::array<float, 600> out {};
    int count = 0;
    adapter.request(true);
    adapter.clock(egs, out.data(), count, 128 * 96 + 11);
    adapter.installWhileQuiescent(false);
    adapter.request(true); // Latest owner intent before the new scan.
    count = 0; // Owner discarded old native/SRC history.
    adapter.clock(egs, out.data(), count, 85 + 3 * 96 + 7);
    require(count == 3 && out[0] == 2 && out[1] == 2 && out[2] == 2
        && egs.clocks == 132 * 96 + 7 && egs.switches == 1 && !egs.partialSwitch
        && adapter.snapshot().phase == 7 && adapter.snapshot().frameGain == 1,
        "cold install retires old ramp, coalesces latest intent and retains all overshoot clocks");
}

struct EgsFixture
{
    std::array<uint8_t, 256> memory {};
    dx7Emu::EGS egs {memory.data()};
    EgsFixture()
    {
        memory.fill(0);
        for (int op = 0; op < 6; ++op)
        {
            const unsigned pitch = ((9000 + op * 37) << 2) | 1u;
            memory[0x20 + op * 2] = uint8_t(pitch >> 8);
            memory[0x21 + op * 2] = uint8_t(pitch);
            egs.update(uint8_t(0x21 + op * 2));
            for (int stage = 0; stage < 4; ++stage)
            {
                memory[0x40 + op * 4 + stage] = 63;
                memory[0x60 + op * 4 + stage] = stage == 3 ? 63 : 0;
            }
            for (int voice = 0; voice < 16; ++voice)
                memory[0x80 + op * 16 + voice] = voice == 0 ? 0 : 255;
            egs.update(uint8_t(0x43 + op * 4));
            egs.update(uint8_t(0x63 + op * 4));
        }
        egs.setAlgorithm(0x30, 31 << 3);
        for (int voice = 0; voice < 16; ++voice) egs.update(uint8_t(voice * 2 + 1));
        memory[0xf1] = 1;
        egs.update(0xf1);
    }
};

static std::vector<float> trace(int partition, bool requests, bool direct)
{
    EgsFixture fixture;
    VDX7NativeSoundMode adapter;
    std::vector<float> result(2000);
    int written = 0;
    constexpr int total = 190000;
    const std::array<int, 6> events {0, 17, 71, 49000, 49617, 50000};
    int position = 0;
    std::size_t event = 0;
    while (position < total)
    {
        if (event < events.size() && position == events[event])
        { if (requests) adapter.request((event % 2) == 0); ++event; }
        const int next = event < events.size() ? events[event] : total;
        const int chunk = std::min({partition, total - position, next - position});
        if (direct) fixture.egs.clock(result.data(), written, chunk);
        else adapter.clock(fixture.egs, result.data(), written, chunk);
        position += chunk;
    }
    require(written == total / 96, "all emitted samples including overshoot retained");
    result.resize(std::size_t(written));
    for (const float sample : result) require(std::isfinite(sample), "finite native output");
    if (requests) require(!adapter.snapshot().activeClean && adapter.snapshot().level == 256,
        "latest Classic request settles");
    return result;
}

// Actual engine lifecycle and native storage, without running fake firmware.
struct VDX7RegressionAccess
{
    static void stableReference(VDX7Engine& engine, bool clean)
    { engine.dx7_.egs.clean(clean); }
    static void lifecycle()
    {
        VDX7Engine engine;
        std::array<float, 8> scratch {};
        int count = 0;
        engine.nativeSoundMode_.clock(engine.dx7_.egs, scratch.data(), count, 17);
        engine.requestSoundMode(true);
        const auto before = engine.soundModeSnapshot();
        engine.resetAudioState();
        engine.prepare(44100);
        engine.resetMidiLifecycle();
        require(engine.soundModeSnapshot() == before, "lifecycle retains actual EGS phase and intent");
        // Buffered samples were generated earlier: consumption must not apply
        // the newly requested ramp a second time or relabel them as Clean.
        engine.nativeBlock_[0] = 0.75f;
        engine.nativePos_ = 0; engine.nativeCount_ = 1;
        engine.dx7_.midiVolume = 7;
        const float result = engine.nextNativeSample();
        require(std::isfinite(result) && engine.soundModeSnapshot() == before,
            "consuming old native storage never advances mode or ramp");
        engine.nativePos_ = 0; engine.nativeCount_ = 1;
        engine.installSoundModeWhileQuiescent(true);
        require(engine.nativeCount_ == 0 && engine.nativePos_ == 0
            && engine.soundModeSnapshot().quiescentInstall
            && engine.soundModeSnapshot().phase == before.phase,
            "quiescent engine install discards old buffer but not actual scan phase");
    }
};

static void ownStimulus(VDX7Engine& engine)
{
    // Own simple carrier stimulus, not copyrighted factory patch data.
    using P = VDX7VoiceData::Parameter;
    using V = VDX7VoiceData::VoiceParameter;
    for (int op = 0; op < 6; ++op)
    {
        for (int field = 0; field < VDX7VoiceData::kParameterCount; ++field)
            engine.setOperatorParameter(op, static_cast<P>(field), 0);
        for (const auto p : {P::rate1, P::rate2, P::rate3, P::rate4}) engine.setOperatorParameter(op, p, 99);
        for (const auto p : {P::level1, P::level2, P::level3}) engine.setOperatorParameter(op, p, 99);
        engine.setOperatorParameter(op, P::outputLevel, op == 0 ? 80 : 0);
        engine.setOperatorParameter(op, P::coarse, 1);
        engine.setOperatorParameter(op, P::detune, 0);
    }
    for (int field = 0; field < VDX7VoiceData::kVoiceParameterCount; ++field)
        engine.setVoiceParameter(static_cast<V>(field), 0);
    for (const auto p : {V::pitchLevel1, V::pitchLevel2, V::pitchLevel3, V::pitchLevel4})
        engine.setVoiceParameter(p, 50);
    engine.setVoiceParameter(V::algorithm, 31);
    engine.setVoiceParameter(V::transpose, 24);
    engine.reloadCurrentProgram();
}

static std::vector<float> hostTimeline(const std::vector<uint8_t>& rom, int rate, int partition)
{
    VDX7Engine engine;
    require(engine.loadRomImage(rom.data(), rom.size()), "partition private boot");
    engine.prepare(rate);
    ownStimulus(engine);
    const std::array<int, 10> events {0, 1001, 1027, 1103, 1400, 2000, 7001, 9000, 14000, 19001};
    std::vector<float> output(32768);
    int position = 0;
    std::size_t event = 0;
    while (position < int(output.size()))
    {
        if (event < events.size() && position == events[event])
        {
            const uint8_t on[] {0x90, 60, 100}, off[] {0x80, 60, 0};
            const uint8_t sustainOn[] {0xb0, 64, 127}, sustainOff[] {0xb0, 64, 0};
            switch (event)
            {
                case 0: engine.handleMidi(on, 3); break;
                case 1: case 3: case 6: engine.requestSoundMode(true); break;
                case 2: case 4: case 8: engine.requestSoundMode(false); break;
                case 5: engine.handleMidi(sustainOn, 3); break;
                case 7: engine.handleMidi(off, 3); break;
                case 9: engine.handleMidi(sustainOff, 3); break;
            }
            ++event;
        }
        const int next = event < events.size() ? events[event] : int(output.size());
        const int count = std::min({partition, int(output.size()) - position, next - position});
        engine.render(output.data() + position, nullptr, count);
        position += count;
    }
    for (const float sample : output) require(std::isfinite(sample), "finite held/sustain/tail transition");
    require(!engine.soundModeSnapshot().activeClean && engine.soundModeSnapshot().level == 256,
        "rapid host requests settle to latest Classic");
    return output;
}

static void privateEngine(const char* path)
{
    std::ifstream file(path, std::ios::binary);
    require(bool(file), "private ROM readable");
    std::vector<uint8_t> rom((std::istreambuf_iterator<char>(file)), {});
    require(rom.size() == VDX7Engine::kFirmwareSize, "private firmware-only fixture");
    for (int rate : {44100, 48000, 96000})
    {
        for (bool clean : {false, true})
        for (bool beforeBoot : {false, true})
        {
            VDX7Engine installed, reference;
            if (beforeBoot) installed.installSoundModeWhileQuiescent(clean);
            for (auto* engine : {&installed, &reference})
            {
                require(engine->loadRomImage(rom.data(), rom.size()), "cold private boot");
                engine->prepare(rate);
                ownStimulus(*engine);
                const uint8_t note[] {0x90, 60, 100};
                engine->handleMidi(note, 3);
            }
            if (!beforeBoot) installed.installSoundModeWhileQuiescent(clean);
            VDX7RegressionAccess::stableReference(reference, clean);
            // Include firmware MIDI admission/envelope onset even at 96 kHz.
            // Still compare from the very first sample, not after a warm-up.
            std::vector<float> cold(32768), direct(32768);
            installed.render(cold.data(), nullptr, int(cold.size()));
            reference.render(direct.data(), nullptr, int(direct.size()));
            require(cold == direct && !installed.soundModeSnapshot().quiescentInstall
                && installed.soundModeSnapshot().activeClean == clean
                && installed.soundModeSnapshot().level == 256,
                "private cold first output matches stable direct EGS mode without startup ramp");
            double peak = 0;
            for (float sample : cold) { require(std::isfinite(sample), "finite cold audio"); peak = std::max(peak, double(std::abs(sample))); }
            require(peak > 0, "cold comparison exercises nonzero own carrier");
        }
        VDX7Engine a, peer, control;
        for (auto* engine : {&a, &peer, &control})
        {
            require(engine->loadRomImage(rom.data(), rom.size()), "private engine boot");
            engine->prepare(rate);
            ownStimulus(*engine);
            const uint8_t note[] {0x90, 60, 100};
            engine->handleMidi(note, 3);
        }
        std::array<float, 257> x {}, y {}, z {};
        double peakClassic = 0, peakClean = 0;
        unsigned differing = 0;
        for (int block = 0; block < 80; ++block)
        {
            if (block == 8) a.requestSoundMode(true);
            if (block == 32) a.requestSoundMode(false);
            a.render(x.data(), nullptr, int(x.size()));
            peer.render(y.data(), nullptr, int(y.size()));
            control.render(z.data(), nullptr, int(z.size()));
            for (std::size_t i = 0; i < x.size(); ++i)
            {
                require(std::isfinite(x[i]), "private Clean host output finite");
                peakClassic = std::max(peakClassic, double(std::abs(y[i])));
                if (block >= 16 && block < 32)
                {
                    peakClean = std::max(peakClean, double(std::abs(x[i])));
                    differing += std::bit_cast<uint32_t>(x[i]) != std::bit_cast<uint32_t>(y[i]);
                }
                require(std::bit_cast<uint32_t>(y[i]) == std::bit_cast<uint32_t>(z[i]),
                    "independent Classic peer unchanged");
                if (block < 8) require(std::bit_cast<uint32_t>(x[i]) == std::bit_cast<uint32_t>(y[i]),
                    "idle engine adapter remains exact Classic");
            }
        }
        require(!a.soundModeSnapshot().activeClean && a.soundModeSnapshot().level == 256,
            "private engine returns to full-gain Classic");
        require(peakClassic > 0 && peakClean > 0 && differing > 0,
            "private stimulus must exercise actual nonzero and different Clean output");
        std::vector<uint8_t> ram;
        require(a.saveRam(ram), "capture RAM before live mode request");
        const int program = a.currentProgram(), bank = a.currentBank(), latency = a.latencySamples();
        a.requestSoundMode(true);
        std::vector<uint8_t> after;
        require(a.saveRam(after) && after == ram && a.currentProgram() == program
            && a.currentBank() == bank && a.latencySamples() == latency,
            "live intent changes no RAM, program, bank or reported latency");
        const auto mode = a.soundModeSnapshot();
        const std::array<uint8_t, 7> bad {};
        require(!a.loadRomImage(bad.data(), bad.size()) && a.soundModeSnapshot() == mode,
            "rejected ROM preserves the native mode trajectory");
        a.prepare(rate);
        require(a.soundModeSnapshot() == mode, "prepare preserves desired mode and actual EGS phase");
        require(a.loadRomImage(rom.data(), rom.size()) && a.soundModeSnapshot().desiredClean,
            "ROM reload retains latest desired mode");
        const auto timeline = hostTimeline(rom, rate, 257);
        for (int partition : {1, 64, 511})
            require(timeline == hostTimeline(rom, rate, partition),
                "actual native/SRC requests and sustain are host-partition invariant");
        std::cout << "PASS: private engine native/SRC transition rate=" << rate
            << " Classic peak=" << peakClassic << " Clean peak=" << peakClean
            << " differing settled samples=" << differing << '\n';
    }
}

int main(int argc, char** argv)
{
    try
    {
        require(argc <= 2, "optional private ROM or negative control only");
        const bool negative = argc == 2 && std::string_view(argv[1]) == "--mid-scan-negative-control";
        ordering(negative);
        quiescentOrdering();
        const auto classic = trace(28, false, false);
        require(classic == trace(28, false, true), "adapter is bit-identical to old EGS clock path");
        const auto switched = trace(28, true, false);
        require(classic != switched, "real EGS Clean signal differs from Classic stimulus");
        for (int partition : {1, 4, 37, 196, 511})
            require(switched == trace(partition, true, false), "absolute-clock requests partition invariant");
        VDX7RegressionAccess::lifecycle();
        if (argc == 2) privateEngine(argv[1]);
        std::cout << "PASS: native scan ordering, muted switch, cancellation, overshoot and lifecycle\n";
    }
    catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
