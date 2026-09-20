#include "PluginProcessor.h"
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

struct VDX7RegressionAccess
{
    static bool held(const VDX7AudioProcessor& p) { return p.engine_.hasHeldMidiNotes(); }
    static void mirror(VDX7AudioProcessor& p) { p.mirrorKeyboardOnMessageThread(); }
    static bool note(const VDX7AudioProcessor& p, int note) { return p.engine_.activeMidiNotes_[note]; }
};

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
        p.processBlock(audio, midi);
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
        p.processBlock(audio, midi);
        const double ms = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - begin).count();
        result.maxMs = std::max(result.maxMs, ms); result.totalMs += ms;
        require(midi.isEmpty(), "instrument consumes MIDI");
        independent.processBlock(silent, empty);
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

int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    if (argc != 2) return 77;
    try
    {
        const juce::File rom(argv[1]);
        checkKeyboard(rom);
        checkLatencyPublication();
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
