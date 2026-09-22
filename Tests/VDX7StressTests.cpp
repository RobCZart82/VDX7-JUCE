#include "PluginProcessor.h"
#include "VDX7AllocationProbe.h"
#include <algorithm>
#include <atomic>
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
    static std::mutex& mutex(VDX7AudioProcessor& p) { return p.engineMutex_; }
    static uint64_t missed(const VDX7AudioProcessor& p) { return p.contendedAudioBlocks_.load(); }
    static uint64_t samples(const VDX7AudioProcessor& p) { return p.contendedAudioSamples_.load(); }
    static uint64_t longestRun(const VDX7AudioProcessor& p) { return p.longestContendedAudioRunSamples_.load(); }
    static uint64_t lastModeMicros(const VDX7AudioProcessor& p) { return p.lastModeTransactionMicros_.load(); }
    static uint64_t peakModeMicros(const VDX7AudioProcessor& p) { return p.peakModeTransactionMicros_.load(); }
};

static void checkEngineContention(const juce::File& rom)
{
    VDX7AudioProcessor p(false);
    require(p.loadRomFromFile(rom), "contention ROM");
    p.prepareToPlay(48000, 256);
    juce::AudioBuffer<float> audio(2, 256);
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 60, juce::uint8(100)), 0);
    processChecked(p, audio, midi);
    require(VDX7RegressionAccess::missed(p) == 0, "ordinary callback has no contention");
    // A deterministic cross-thread transaction, not a scheduler-dependent race.
    {
        std::unique_lock lock(VDX7RegressionAccess::mutex(p));
        auto callback = std::async(std::launch::async, [&] {
            for (int n = 0; n < 3; ++n) {
                processChecked(p, audio, midi);
                require(audio.getMagnitude(0, 256) == 0, "contended callback is silent");
            }
        });
        const bool blocked = callback.wait_for(std::chrono::seconds(1)) == std::future_status::timeout;
        lock.unlock();
        callback.get();
        require(!blocked, "audio must not wait for engine lock");
    }
    require(VDX7RegressionAccess::missed(p) == 3 && VDX7RegressionAccess::samples(p) == 768,
            "exact lost block/sample diagnostics");
    require(VDX7RegressionAccess::longestRun(p) == 768,
            "exact longest contiguous loss diagnostic");
    processChecked(p, audio, midi);
    require(VDX7RegressionAccess::missed(p) == 3, "successful callback does not increment loss");
    std::cout << "Measured forced contention: 3 blocks, 768 samples = 16 ms at 48 kHz\n";

    // Real wall-clock UI operations, deliberately no deadline assertions. The
    // processor also records only the work performed while it owns its lock.
    for (int mode : {1, 0}) {
        const auto begin = std::chrono::steady_clock::now();
        require(p.setPlaySettingFromUi(0, mode), "measured mode switch");
        const auto ms = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - begin).count();
        const auto lockMicros = VDX7RegressionAccess::lastModeMicros(p);
        require(lockMicros > 0 && VDX7RegressionAccess::peakModeMicros(p) >= lockMicros,
                "mode transaction diagnostics");
        std::cout << "Mode switch " << mode << " callerWallMs=" << ms
                  << " lockWorkMicros=" << lockMicros << '\n';
    }
    const auto begin = std::chrono::steady_clock::now();
    for (int n = 0; n < 1000; ++n) {
        (void)p.getControllerSettings(); (void)p.getPlaySettings(); (void)p.getPitchBendSettings();
    }
    std::cout << "1000 performance reads wallMs="
              << std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - begin).count()
              << " (uncontended diagnostic, not a realtime guarantee)\n";
}

static void checkPerformanceDisplay(const juce::File& rom)
{
    VDX7AudioProcessor p(false);
    require(p.loadRomFromFile(rom), "display ROM");
    p.prepareToPlay(48000, 256);
    auto compare = [&] {
        const auto display = p.getPerformanceDisplay();
        require(display.controllers == p.getControllerSettings(), "display controller values");
        require(display.play == p.getPlaySettings(), "display play values");
        require(display.bend == p.getPitchBendSettings(), "display bend values");
    };
    compare();
    for (int value : {0, 99}) {
        for (int c = 0; c < 4; ++c) {
            require(p.setControllerSettingFromUi(c, 0, value), "display range update");
            for (int f = 1; f < 4; ++f)
                require(p.setControllerSettingFromUi(c, f, (c + f + value) % 2), "display assignment update");
        }
        require(p.setPlaySettingFromUi(3, value), "display portamento time");
        for (int f = 0; f < 2; ++f)
            require(p.setPitchBendSettingFromUi(f, value == 0 ? 0 : 12), "display bend update");
        compare();
    }
    juce::MemoryBlock state;
    p.getStateInformation(state);
    p.setControllerSettingFromUi(0, 0, 1);
    p.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
    compare();
    // Display reads must finish even when another thread owns the engine lock.
    {
        std::unique_lock lock(VDX7RegressionAccess::mutex(p));
        auto reader = std::async(std::launch::async, [&] {
            for (int n = 0; n < 10000; ++n) {
                require(p.getPerformanceDisplay().controllers[0] == 99, "stable display frame");
                require(p.getControllerSettings()[0] == 99, "legacy controller read is snapshot-backed");
                require(p.getPlaySettings()[3] == 99, "legacy play read is snapshot-backed");
                require(p.getPitchBendSettings()[0] == 12, "legacy bend read is snapshot-backed");
            }
        });
        const bool blocked = reader.wait_for(std::chrono::seconds(1)) == std::future_status::timeout;
        lock.unlock(); reader.get();
        require(!blocked, "performance display reads must not acquire engine lock");
    }
    juce::AudioBuffer<float> audio(2, 256);
    juce::MidiBuffer midi;
    for (int cc : {126, 127}) {
        midi.addEvent(juce::MidiMessage::controllerEvent(1, cc, 0), 0);
        for (int n = 0; n < 100; ++n) processChecked(p, audio, midi);
        compare(); // Firmware-driven mode changes publish after rendering too.
    }
    std::cout << "PASS: lock-free coherent performance display, settings, restore and MIDI mode refresh\n";
}

// Frequent controller/tuning edits and the bounded three-byte portamento-time
// write are coalesced in atomics and committed by the next engine-owning path.
// POLY/MONO remains an explicit firmware reset transaction outside this probe.
static void checkCoalescedPerformanceWrites(const juce::File& rom)
{
    VDX7AudioProcessor p(false);
    require(p.loadRomFromFile(rom), "coalesced settings ROM");
    {
        std::unique_lock lock(VDX7RegressionAccess::mutex(p));
        auto writer = std::async(std::launch::async, [&] {
            require(p.setControllerSettingFromUi(0, 0, 77), "coalesced controller request");
            require(p.setControllerSettingFromUi(3, 3, 1), "coalesced assignment request");
            require(p.setPlaySettingFromUi(1, 1), "coalesced portamento mode request");
            require(p.setPlaySettingFromUi(2, 1), "coalesced glissando request");
            require(p.setPlaySettingFromUi(3, 91), "coalesced portamento time request");
            require(p.setPitchBendSettingFromUi(0, 12), "coalesced bend request");
            require(p.setMasterTuneFromUi(-123), "coalesced tune request");
        });
        const bool blocked = writer.wait_for(std::chrono::seconds(1)) == std::future_status::timeout;
        lock.unlock(); writer.get();
        require(!blocked, "ordinary global settings must not acquire engine lock");
    }
    require(p.getControllerSettings()[0] == 77 && p.getControllerSettings()[15] == 1,
            "coalesced controller display is immediate");
    require(p.getPlaySettings()[1] == 1 && p.getPlaySettings()[2] == 1
                && p.getPlaySettings()[3] == 91,
            "coalesced play display is immediate");
    require(p.getPitchBendSettings()[0] == 12 && p.getMasterTune() == -123,
            "coalesced bend/tune display is immediate");
    juce::MemoryBlock saved;
    p.getStateInformation(saved); // Must commit queued global settings before project capture.
    auto& engine = VDX7RegressionAccess::engine(p);
    require(engine.getControllerSetting(0, 0) == 77 && engine.getControllerSetting(3, 3) == 1,
            "state capture commits controller settings");
    require(engine.getPlaySetting(1) == 1 && engine.getPlaySetting(2) == 1
                && engine.getPlaySetting(3) == 91,
            "state capture commits play settings");
    require(engine.getPitchBendSetting(0) == 12 && engine.masterTune() == -123,
            "state capture commits bend and tuning");

    VDX7AudioProcessor overlap(false);
    require(overlap.loadRomFromFile(rom), "coalesced overlap ROM");
    overlap.prepareToPlay(48000, 256);
    juce::AudioBuffer<float> audio(2, 256);
    juce::MidiBuffer midi;
    std::atomic<bool> writerDone { false };
    std::thread writer([&] {
        for (int n = 0; n < 1000; ++n)
        {
            overlap.setControllerSettingFromUi(0, 0, n % 100);
            overlap.setControllerSettingFromUi(1, 1, n & 1);
            overlap.setPlaySettingFromUi(1, n & 1);
            overlap.setPlaySettingFromUi(2, (n >> 1) & 1);
            overlap.setPlaySettingFromUi(3, n % 100);
            overlap.setPitchBendSettingFromUi(0, n % 13);
            overlap.setMasterTuneFromUi(-256 + (n % 512));
        }
        writerDone.store(true, std::memory_order_release);
    });
    while (!writerDone.load(std::memory_order_acquire))
    {
        processChecked(overlap, audio, midi);
    }
    writer.join();
    processChecked(overlap, audio, midi);
    const auto& overlapEngine = VDX7RegressionAccess::engine(overlap);
    require(VDX7RegressionAccess::missed(overlap) == 0,
            "coalesced global edits do not contend with audio callbacks");
    require(overlapEngine.getControllerSetting(0, 0) == 99
            && overlapEngine.getControllerSetting(1, 1) == 1
            && overlapEngine.getPlaySetting(1) == 1
            && overlapEngine.getPlaySetting(2) == 1
            && overlapEngine.getPlaySetting(3) == 99
            && overlapEngine.getPitchBendSetting(0) == 11
            && overlapEngine.masterTune() == 231,
            "coalesced overlap commits the latest values");

    // A portamento-time write must survive a serial-overflow recovery. The
    // first audio block retries while recovery owns the FIFO; a later block
    // commits the latest requested value without asking the UI to retry.
    VDX7AudioProcessor retry(false);
    require(retry.loadRomFromFile(rom), "coalesced retry ROM");
    retry.prepareToPlay(48000, 256);
    auto& retryEngine = VDX7RegressionAccess::engine(retry);
    const uint8_t bend[] {0xe0, 0, 64};
    for (int n = 0; n < 3000; ++n) retryEngine.handleMidi(bend, 3);
    require(retryEngine.isMidiRecovering(), "coalesced retry enters serial recovery");
    require(retry.setPlaySettingFromUi(3, 73), "coalesced retry time request");
    juce::AudioBuffer<float> retryAudio(2, 256);
    juce::MidiBuffer retryMidi;
    for (int block = 0; block < 1200 && retryEngine.getPlaySetting(3) != 73; ++block)
        processChecked(retry, retryAudio, retryMidi);
    require(!retryEngine.isMidiRecovering() && retryEngine.getPlaySetting(3) == 73,
            "coalesced time retries after serial recovery");
    std::cout << "PASS: coalesced global UI writes avoid engine-lock audio contention\n";
}

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
        checkEngineContention(rom);
        checkPerformanceDisplay(rom);
        checkCoalescedPerformanceWrites(rom);
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
