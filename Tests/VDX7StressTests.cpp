#include "PluginProcessor.h"
#include "VDX7AllocationProbe.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <future>
#include <stdexcept>
#include <thread>
#include <vector>

static void require(bool ok, const char* message)
{ if (!ok) throw std::runtime_error(message); }

static void processChecked(VDX7AudioProcessor& p, juce::AudioBuffer<float>& audio, juce::MidiBuffer& midi)
{
    using namespace VDX7AllocationProbe;
    allocations = deallocations = 0;
    enabled = true;
    try { p.processBlock(audio, midi); }
    catch (...) { enabled = false; throw; }
    enabled = false;
    require(allocations == 0 && deallocations == 0, "ordinary C++ allocation/deallocation in audio callback");
}

struct VDX7RegressionAccess
{
    static bool held(const VDX7AudioProcessor& p) { return p.engine_.hasHeldMidiNotes(); }
    static void mirror(VDX7AudioProcessor& p) { p.mirrorKeyboardOnMessageThread(); }
    static bool note(const VDX7AudioProcessor& p, int note) { return p.engine_.activeMidiNotes_[note]; }
    static uint64_t overloads(const VDX7AudioProcessor& p) { return p.engine_.midiOverloadCount(); }
    static VDX7Engine& engine(VDX7AudioProcessor& p) { return p.engine_; }
};

static void checkCapacityAndPendingOff(const juce::File& rom)
{
    for (int mode : {0, 1})
    for (bool samePitch : {false, true})
    for (bool overload : {false, true})
    {
        VDX7AudioProcessor p(false);
        require(p.loadRomFromFile(rom), "capacity ROM");
        p.prepareToPlay(48000, 256);
        auto& e = VDX7RegressionAccess::engine(p);
        using P = VDX7VoiceData::Parameter;
        using V = VDX7VoiceData::VoiceParameter;
        e.setVoiceParameter(V::algorithm, 31);
        e.setVoiceParameter(V::feedback, 0);
        for (int op = 0; op < 6; ++op)
        {
            e.setOperatorParameter(op, P::outputLevel, op == 0 ? 99 : 0);
            for (auto f : {P::rate1, P::rate2, P::rate3, P::rate4}) e.setOperatorParameter(op, f, 99);
            for (auto f : {P::level1, P::level2, P::level3}) e.setOperatorParameter(op, f, 99);
            e.setOperatorParameter(op, P::level4, 0);
        }
        e.reloadCurrentProgram();
        juce::AudioBuffer<float> audio(2, 256);
        juce::MidiBuffer midi;
        auto render = [&](int blocks) {
            float peak = 0;
            for (int b = 0; b < blocks; ++b) {
                processChecked(p, audio, midi);
                const float level = audio.getMagnitude(0, 256);
                require(std::isfinite(level), "capacity finite audio");
                if (b >= blocks / 2) peak = std::max(peak, level);
            }
            return peak;
        };
        render(40);
        require(e.setPlaySetting(0, mode), "capacity play mode");
        // Let firmware allocate each voice; this is not merely a queued burst.
        for (int n = 0; n < 32; ++n) {
            uint8_t on[] {static_cast<uint8_t>(0x90 | (n % 16)),
                          static_cast<uint8_t>(samePitch ? 60 : 48 + n), 100};
            e.handleMidi(on, 3); render(8);
        }
        require(render(40) > 1e-5f, "capacity produces sustained audio");
        uint8_t pedal[] {0xb0, 64, 127};
        e.handleMidi(pedal, 3); render(8);
        // Ownership changes immediately, but firmware has not consumed these
        // offs when the following burst causes serial overflow and flushes RX.
        for (int n = 0; n < 32; ++n) {
            uint8_t off[] {static_cast<uint8_t>(0x80 | (n % 16)),
                           static_cast<uint8_t>(samePitch ? 60 : 48 + n), 0};
            e.handleMidi(off, 3);
        }
        if (overload) {
            uint8_t bend[] {0xe0, 0, 64};
            for (int n = 0; n < 3000; ++n) e.handleMidi(bend, 3);
            require(e.midiOverloadCount() == 1, "pending-off overflow triggered");
        } else e.allNotesOff();
        require(render(800) < 1e-4f, "no stuck voice after capacity/pending-off recovery");
        require(!e.hasHeldMidiNotes() && !e.isMidiRecovering(), "capacity ownership/recovery clear");
        uint8_t fresh[] {0x90, 65, 100}, release[] {0x80, 65, 0};
        e.handleMidi(fresh, 3);
        require(render(100) > 1e-5f, "fresh note audible after capacity recovery");
        e.handleMidi(release, 3);
        require(render(400) < 1e-4f && !e.hasHeldMidiNotes(), "fresh note releases after recovery");
        std::cout << "PASS: capacity mode=" << mode << " samePitch=" << samePitch
                  << " pendingOffOverflow=" << overload << '\n';
    }
}

struct StressResult { std::vector<float> audio; double maxMs = 0, totalMs = 0; };

// Deliberately stall a UI listener while JUCE holds its keyboard-state lock.
// Audio must complete without needing that UI lock. Generous timeout is a
// deadlock regression guard, not a claimed realtime deadline.
static void probeKeyboardLock()
{
    VDX7AudioProcessor p(false);
    struct Listener final : juce::MidiKeyboardState::Listener
    {
        std::promise<void> entered, release;
        void handleNoteOn(juce::MidiKeyboardState*, int, int, float) override
        { entered.set_value(); release.get_future().wait(); }
        void handleNoteOff(juce::MidiKeyboardState*, int, int, float) override {}
    } listener;
    p.keyboardState().addListener(&listener);
    std::thread ui([&] { p.keyboardState().noteOn(1, 60, 1.0f); });
    listener.entered.get_future().wait();
    std::promise<void> audioStarted;
    auto callback = std::async(std::launch::async, [&]
    {
        juce::AudioBuffer<float> audio(2, 64);
        juce::MidiBuffer midi;
        audioStarted.set_value();
        processChecked(p, audio, midi);
    });
    audioStarted.get_future().wait();
    const bool blocked = callback.wait_for(std::chrono::seconds(1)) == std::future_status::timeout;
    listener.release.set_value();
    ui.join(); callback.get();
    p.keyboardState().removeListener(&listener);
    require(!blocked, "audio callback waits for keyboard UI lock");
    std::cout << "PASS: audio completed while keyboard UI lock remained held\n";
}

static void checkKeyboard(const juce::File& rom)
{
    VDX7AudioProcessor p(false);
    require(p.loadRomFromFile(rom), "keyboard ROM");
    p.prepareToPlay(48000, 64);
    juce::AudioBuffer<float> audio(2, 64);
    juce::MidiBuffer midi;
    p.keyboardState().noteOn(1, 60, 0.8f);
    VDX7RegressionAccess::mirror(p);
    require(p.keyboardState().isNoteOn(1, 60), "mirror must not erase pending UI note");
    p.processBlock(audio, midi);
    require(VDX7RegressionAccess::note(p, 60), "UI note reaches firmware");
    p.keyboardState().noteOff(1, 60, 0);
    p.processBlock(audio, midi);
    require(!VDX7RegressionAccess::note(p, 60), "UI release reaches firmware");
    midi.addEvent(juce::MidiMessage::noteOn(2, 67, juce::uint8(100)), 17);
    p.processBlock(audio, midi);
    VDX7RegressionAccess::mirror(p);
    require(p.keyboardState().isNoteOn(2, 67), "host note mirrored to keyboard");
    midi.addEvent(juce::MidiMessage::noteOff(2, 67), 12);
    p.processBlock(audio, midi);
    VDX7RegressionAccess::mirror(p);
    require(!p.keyboardState().isNoteOn(2, 67), "host release mirrored without feedback");
    require(!VDX7RegressionAccess::held(p), "keyboard mirror must not inject duplicate notes");
    for (int i = 0; i < 300; ++i)
    {
        p.keyboardState().noteOn(1, 60, 0.8f);
        p.keyboardState().noteOff(1, 60, 0);
    }
    p.processBlock(audio, midi);
    require(!VDX7RegressionAccess::held(p), "keyboard overflow reconciles notes");
    p.keyboardState().noteOn(1, 61, 0.8f);
    p.processBlock(audio, midi);
    require(VDX7RegressionAccess::note(p, 61), "keyboard recovers after overflow");
    p.keyboardState().noteOff(1, 61, 0);
    p.processBlock(audio, midi);
}

static void checkLatencyPublication()
{
    VDX7AudioProcessor p(false);
    struct Listener final : juce::AudioProcessorListener
    {
        int calls = 0;
        void audioProcessorParameterChanged(juce::AudioProcessor*, int, float) override {}
        void audioProcessorChanged(juce::AudioProcessor* processor, const ChangeDetails& details) override
        {
            if (!details.latencyChanged) return;
            juce::MemoryBlock state;
            processor->getStateInformation(state); // Must not deadlock on engineMutex_.
            require(state.getSize() != 0, "reentrant latency state capture");
            ++calls;
        }
    } listener;
    p.addListener(&listener);
    for (int rate : {44100, 48000, 96000}) p.prepareToPlay(rate, 64);
    p.removeListener(&listener);
    require(listener.calls == 3, "latency change notification at each rate");
}

// Identical absolute MIDI/automation timeline at every host partition.
// Timings are diagnostic wall times, NOT a realtime deadline assertion.
static StressResult run(const juce::File& rom, int rate, int block)
{
    VDX7AudioProcessor p(false), independent(false);
    require(p.loadRomFromFile(rom) && independent.loadRomFromFile(rom), "load test ROM");
    p.prepareToPlay(rate, block);
    require(p.getLatencySamples() == int(std::ceil(128.0 * rate / VDX7Engine::kNativeSampleRate)),
            "host receives exact SRC latency");
    independent.prepareToPlay(rate, block);
    juce::AudioBuffer<float> audio(2, block), silent(2, block);
    juce::MidiBuffer midi, empty;
    midi.ensureSize(32768);
    auto* feedback = p.parameters().getParameter(
        VDX7ParameterIDs::voiceParameter(VDX7VoiceData::VoiceParameter::feedback));
    require(feedback != nullptr, "feedback parameter");
    StressResult result;
    const int duration = ((rate / 2 + 511) / 512) * 512;
    const int scale = rate == 96000 ? 2 : 1;
    result.audio.resize(static_cast<std::size_t>(duration));
    for (int start = 0; start < duration; start += block)
    {
        midi.clear();
        auto add = [&](int at, const juce::MidiMessage& m)
        { at *= scale; if (at >= start && at < start + block) midi.addEvent(m, at - start); };
        add(512, juce::MidiMessage::programChange(1, 0));
        add(1024, juce::MidiMessage::noteOn(1, 60, juce::uint8(100)));
        add(3500, juce::MidiMessage::noteOff(1, 60));
        add(4096, juce::MidiMessage::controllerEvent(1, 32, 1));
        add(4608, juce::MidiMessage::programChange(1, 5));
        // Repeated 16-note chords, with non-block-aligned on/off positions.
        for (int chord = 0; chord < 4; ++chord)
            for (int note = 0; note < 16; ++note)
            {
                add(6000 + chord * 2048 + note * 3,
                    juce::MidiMessage::noteOn(1, 48 + note, juce::uint8(100)));
                add(7400 + chord * 2048 + note * 3,
                    juce::MidiMessage::noteOff(1, 48 + note));
            }
        add(6100, juce::MidiMessage::controllerEvent(1, 64, 127));
        add(15003, juce::MidiMessage::controllerEvent(1, 64, 0));
        for (int at = 6200; at < 14000; at += 37)
        {
            add(at, juce::MidiMessage::controllerEvent(1, 1, (at / 37) % 128));
            add(at, juce::MidiMessage::pitchWheel(1, 8192 + (at % 1024)));
        }
        if (start >= 6144 * scale && start <= 12288 * scale && start % (1024 * scale) == 0)
            feedback->setValueNotifyingHost(feedback->convertTo0to1(float((start / (1024 * scale)) % 8)));
        const auto begin = std::chrono::steady_clock::now();
        processChecked(p, audio, midi);
        const double ms = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - begin).count();
        result.maxMs = std::max(result.maxMs, ms); result.totalMs += ms;
        require(midi.isEmpty(), "instrument consumes MIDI");
        processChecked(independent, silent, empty);
        require(silent.getMagnitude(0, block) < 1.0e-5f, "instances remain isolated");
        for (int i = 0; i < block; ++i)
        {
            const float left = audio.getSample(0, i), right = audio.getSample(1, i);
            require(std::isfinite(left) && left == right, "finite dual-mono output");
            result.audio[static_cast<std::size_t>(start + i)] = left;
        }
    }
    require(!VDX7RegressionAccess::held(p), "all chord notes and sustain released");
    require(std::any_of(result.audio.begin(), result.audio.end(),
                       [](float v) { return std::abs(v) > 1.0e-5f; }), "stress timeline produces sound");
    require(p.getCurrentBank() == 1 && p.getCurrentProgram() == 5, "selection survives stress");
    require(independent.getCurrentBank() == 0 && independent.getCurrentProgram() == 0,
            "independent selection unchanged");
    return result;
}

static void checkLongRunAndOverload(const juce::File& rom)
{
    VDX7AudioProcessor p(false);
    require(p.loadRomFromFile(rom), "long-run ROM");
    p.prepareToPlay(48000, 256);
    juce::AudioBuffer<float> audio(2, 256);
    juce::MidiBuffer midi;
    midi.ensureSize(131072); // Host setup, deliberately outside measured callback.
    auto* feedback = p.parameters().getParameter(
        VDX7ParameterIDs::voiceParameter(VDX7VoiceData::VoiceParameter::feedback));
    for (int block = 0; block < 11250; ++block) // 60 seconds of simulated audio.
    {
        if (block % 32 == 0)
        {
            for (int n = 48; n < 64; ++n)
                midi.addEvent(juce::MidiMessage::noteOn(1, n, juce::uint8(100)), n - 48);
            feedback->setValueNotifyingHost(feedback->convertTo0to1(float((block / 32) % 8)));
        }
        if (block % 32 == 24)
            for (int n = 48; n < 64; ++n) midi.addEvent(juce::MidiMessage::noteOff(1, n), n - 48);
        midi.addEvent(juce::MidiMessage::controllerEvent(1, 1, block % 128), 20);
        processChecked(p, audio, midi);
        require(std::isfinite(audio.getMagnitude(0, 256)), "long-run finite output");
    }
    require(VDX7RegressionAccess::overloads(p) == 0, "ordinary sustained load must not overload");
    // Each burst is intentionally far above either queue capacity. Run both
    // serial and controller cases; a later note-off must never be lost silently.
    for (int kind = 0; kind < 2; ++kind)
    {
        midi.addEvent(juce::MidiMessage::controllerEvent(1, 64, 127), 0);
        for (int i = 0; i < 5000; ++i)
            midi.addEvent(kind == 0 ? juce::MidiMessage::noteOn(1, 60, juce::uint8(100))
                                   : juce::MidiMessage::controllerEvent(1, 1, i % 128), 0);
        midi.addEvent(juce::MidiMessage::noteOff(1, 60), 0);
        midi.addEvent(juce::MidiMessage::controllerEvent(1, 64, 0), 0);
        processChecked(p, audio, midi);
        require(!VDX7RegressionAccess::held(p), "overload reconciles held notes and pedal");
        for (int block = 0; block < 600; ++block) processChecked(p, audio, midi);
        require(audio.getMagnitude(0, 256) < 1e-4f, "firmware voices release after overload");
    }
    require(VDX7RegressionAccess::overloads(p) == 2, "both overload paths counted");
    require(p.getStatusText().contains("MIDI overload"), "overload is visible in status");
    midi.addEvent(juce::MidiMessage::noteOn(1, 60, juce::uint8(100)), 0);
    float peak = 0;
    for (int block = 0; block < 100; ++block)
    { processChecked(p, audio, midi); peak = std::max(peak, audio.getMagnitude(0, 256)); }
    require(peak > 1e-5f && VDX7RegressionAccess::held(p), "fresh audible note after overload");
    std::cout << "PASS: 60-second simulated load, serial/controller overflow recovery, callback C++ heap probe\n";
}

int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    if (argc != 2) return 77;
    try
    {
        const juce::File rom(argv[1]);
        VDX7AllocationProbe::enabled = true;
        auto* allocation = ::operator new(16);
        ::operator delete(allocation);
        VDX7AllocationProbe::enabled = false;
        require(VDX7AllocationProbe::allocations == 1 && VDX7AllocationProbe::deallocations == 1,
                "allocation probe self-test");
        checkKeyboard(rom);
        checkCapacityAndPendingOff(rom);
        checkLatencyPublication();
        checkLongRunAndOverload(rom);
        for (int rate : {44100, 48000, 96000})
        {
            std::vector<float> reference;
            for (int block : {64, 128, 256, 512})
            {
                const auto result = run(rom, rate, block);
                if (reference.empty()) reference = result.audio;
                else require(reference == result.audio, "host partition changes stress audio");
                std::cout << "rate=" << rate << " block=" << block
                          << " maxCallbackMs=" << result.maxMs
                          << " aggregateCallbackMs=" << result.totalMs << '\n';
            }
        }
        probeKeyboardLock();
        std::cout << "PASS: 12 configurations, dense MIDI/automation, two-instance isolation, exact partition invariance\n";
    }
    catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
