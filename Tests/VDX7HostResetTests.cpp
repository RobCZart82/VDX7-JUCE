// N1 host-reset regression proposal. Execute with the user's local ROM only.
// It intentionally uses the public reset() entry point, not releaseResources().
#include "PluginProcessor.h"
#include "VDX7AllocationProbe.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <chrono>
#include <future>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

static void require(bool ok, const char* message)
{
    if (!ok) throw std::runtime_error(message);
}

struct VDX7RegressionAccess
{
    static VDX7Engine& engine(VDX7AudioProcessor& p) { return p.engine_; }
    static std::mutex& mutex(VDX7AudioProcessor& p) { return p.engineMutex_; }
    static bool deferred(const VDX7AudioProcessor& p) { return p.deferredMidi_.active(); }
    static uint32_t dirty(const VDX7AudioProcessor& p) { return p.modifiedVoices_.load(); }
};

static void resetChecked(VDX7AudioProcessor& p)
{
    using namespace VDX7AllocationProbe;
    allocations = deallocations = 0;
    enabled = true;
    p.reset();
    enabled = false;
    require(allocations == 0 && deallocations == 0, "reset allocates/deallocates ordinary C++ storage");
}

static void processChecked(VDX7AudioProcessor& p, juce::AudioBuffer<float>& audio,
                           juce::MidiBuffer& midi)
{
    using namespace VDX7AllocationProbe;
    allocations = deallocations = 0;
    enabled = true;
    p.processBlock(audio, midi);
    enabled = false;
    require(allocations == 0 && deallocations == 0, "reset callback allocates/deallocates ordinary C++ storage");
    for (int ch = 0; ch < audio.getNumChannels(); ++ch)
        for (int i = 0; i < audio.getNumSamples(); ++i)
            require(std::isfinite(audio.getSample(ch, i)), "non-finite reset output");
    require(midi.isEmpty(), "instrument must consume MIDI");
}

static void initialise(VDX7AudioProcessor& p, const juce::File& rom, int rate, int block)
{
    require(p.loadRomFromFile(rom), "load explicit local ROM");
    p.prepareToPlay(rate, block);
    p.selectProgramFromUi(3);
    juce::MemoryBlock state;
    p.getStateInformation(state); // Commit the program selection.
    auto& e = VDX7RegressionAccess::engine(p);
    using P = VDX7VoiceData::Parameter;
    using V = VDX7VoiceData::VoiceParameter;
    e.setVoiceParameter(V::algorithm, 32);
    e.setVoiceParameter(V::feedback, 0);
    e.setVoiceParameter(V::transpose, 0);
    e.setVoiceParameter(V::pitchModDepth, 0);
    e.setVoiceParameter(V::amplitudeModDepth, 0);
    for (auto param : {V::pitchLevel1, V::pitchLevel2, V::pitchLevel3, V::pitchLevel4})
        e.setVoiceParameter(param, 50);
    for (int op = 0; op < 6; ++op)
    {
        e.setOperatorParameter(op, P::outputLevel, op == 0 ? 99 : 0);
        e.setOperatorParameter(op, P::oscillatorMode, 0);
        e.setOperatorParameter(op, P::coarse, 1);
        e.setOperatorParameter(op, P::fine, 0);
        e.setOperatorParameter(op, P::detune, 0);
        e.setOperatorParameter(op, P::leftScaleDepth, 0);
        e.setOperatorParameter(op, P::rightScaleDepth, 0);
        e.setOperatorParameter(op, P::rateScaling, 0);
        e.setOperatorParameter(op, P::velocitySensitivity, 0);
        e.setOperatorParameter(op, P::amplitudeModSensitivity, 0);
        for (auto param : {P::rate1, P::rate2, P::rate3}) e.setOperatorParameter(op, param, 99);
        e.setOperatorParameter(op, P::rate4, 1); // Long tail: allNotesOff alone is insufficient.
        for (auto param : {P::level1, P::level2, P::level3}) e.setOperatorParameter(op, param, 99);
        e.setOperatorParameter(op, P::level4, 0);
    }
    e.setMasterTune(123);
    e.setControllerSetting(0, 0, 37);
    e.setPitchBendSetting(0, 6);
    e.reloadCurrentProgram();
    p.synchroniseOperatorParametersFromEngine();
    juce::AudioBuffer<float> audio(2, block);
    juce::MidiBuffer midi;
    for (int n = 0; n < rate / block / 4; ++n) processChecked(p, audio, midi);
}

struct SavedSettings
{
    std::vector<uint8_t> voices;
    std::array<int, 16> controllers {};
    std::array<int, 4> play {};
    std::array<int, 2> bend {};
    std::vector<float> parameters;
    int bank = -1, program = 0, tuning = 0, channel = 0;
    uint32_t dirty = 0;
};

static SavedSettings capture(VDX7AudioProcessor& p)
{
    SavedSettings s;
    auto& e = VDX7RegressionAccess::engine(p);
    require(e.saveRam(s.voices), "capture RAM");
    s.voices.resize(4096); // Runtime RAM is not expected to be byte-identical.
    for (int c = 0; c < 4; ++c)
        for (int f = 0; f < 4; ++f) s.controllers[c * 4 + f] = e.getControllerSetting(c, f);
    for (int f = 0; f < 4; ++f) s.play[f] = e.getPlaySetting(f);
    for (int f = 0; f < 2; ++f) s.bend[f] = e.getPitchBendSetting(f);
    for (auto* param : p.getParameters()) s.parameters.push_back(param->getValue());
    s.bank = e.currentBank(); s.program = e.currentProgram(); s.tuning = e.masterTune();
    s.channel = p.getMidiInputChannel(); s.dirty = VDX7RegressionAccess::dirty(p);
    return s;
}

static void unchanged(const SavedSettings& a, const SavedSettings& b)
{
    require(a.voices == b.voices && a.bank == b.bank && a.program == b.program,
            "reset changed the working voice bank/selection");
    require(a.controllers == b.controllers && a.play == b.play && a.bend == b.bend
            && a.tuning == b.tuning && a.channel == b.channel,
            "reset changed persistent performance/settings values");
    require(a.parameters == b.parameters && a.dirty == b.dirty,
            "reset changed parameters or voice export markers");
}

static void testHeldAndTail(const juce::File& rom, int rate, int block, bool nonzeroReleaseFloor = false)
{
    auto owner = std::make_unique<VDX7AudioProcessor>(false);
    auto& p = *owner;
    initialise(p, rom, rate, block);
    if (nonzeroReleaseFloor)
    {
        auto& e = VDX7RegressionAccess::engine(p);
        e.setOperatorParameter(0, VDX7VoiceData::Parameter::level4, 70);
        e.reloadCurrentProgram();
        p.synchroniseOperatorParametersFromEngine();
    }
    const auto before = capture(p);
    juce::AudioBuffer<float> audio(2, block);
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::controllerEvent(1, 64, 127), 0);
    midi.addEvent(juce::MidiMessage::noteOn(1, 60, juce::uint8(100)), 1);
    float peak = 0;
    for (int n = 0; n < rate / block / 4; ++n)
    { processChecked(p, audio, midi); peak = std::max(peak, audio.getMagnitude(0, block)); }
    require(peak > 1e-4f, "held-note fixture is silent");
    require(VDX7RegressionAccess::engine(p).hasHeldMidiNotes(), "fixture needs held ownership");
    resetChecked(p);
    resetChecked(p); // Repeated requests are coalesced, not lost.
    juce::AudioBuffer<float> zero(2, 0);
    processChecked(p, zero, midi); // A zero-sample callback must not lose the request.
    for (int n = 0; n < rate / block; ++n)
    {
        processChecked(p, audio, midi);
        require(audio.getMagnitude(0, block) < 1e-5f, "host reset left held audio or a release tail");
    }
    require(!VDX7RegressionAccess::engine(p).hasHeldMidiNotes(), "host reset left note/sustain ownership");
    unchanged(before, capture(p));
    midi.addEvent(juce::MidiMessage::noteOn(1, 72, juce::uint8(100)), 0);
    peak = 0;
    for (int n = 0; n < rate / block / 4; ++n)
    { processChecked(p, audio, midi); peak = std::max(peak, audio.getMagnitude(0, block)); }
    require(peak > 1e-4f, "new note does not play after reset");
    midi.addEvent(juce::MidiMessage::noteOff(1, 72), 0);
    processChecked(p, audio, midi);
    require(!VDX7RegressionAccess::engine(p).hasHeldMidiNotes(),
            "post-reset Note Off left held ownership");
    // This fixture deliberately has a slow (or nonzero-floor) release, so
    // ownership release must not be confused with immediate audio silence.
    std::cout << "PASS: reset held note, sustain, tail, repeated/zero-block requests, state and fresh note at "
              << rate << '/' << block << '\n';
}

static void testFreshNoteRelease(const juce::File& rom, int rate, int block)
{
    auto owner = std::make_unique<VDX7AudioProcessor>(false);
    auto& p = *owner;
    initialise(p, rom, rate, block);
    auto& e = VDX7RegressionAccess::engine(p);
    e.setOperatorParameter(0, VDX7VoiceData::Parameter::rate4, 99);
    e.reloadCurrentProgram();
    p.synchroniseOperatorParametersFromEngine();
    juce::AudioBuffer<float> audio(2, block);
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::controllerEvent(1, 64, 127), 0);
    midi.addEvent(juce::MidiMessage::noteOn(1, 60, juce::uint8(100)), 1);
    float peak = 0;
    for (int n = 0; n < rate / block / 4; ++n)
    { processChecked(p, audio, midi); peak = std::max(peak, audio.getMagnitude(0, block)); }
    require(peak > 1e-4f && e.hasHeldMidiNotes(), "release fixture must sound before reset");
    const auto before = capture(p);
    resetChecked(p);
    // Submit the new note immediately, while reset may still be draining.
    // Do not send sustain-off: reset itself must reconcile the old pedal.
    midi.addEvent(juce::MidiMessage::noteOn(1, 72, juce::uint8(100)), 0);
    int firstAudibleBlock = -1;
    for (int n = 0; n < (rate * 2 + block - 1) / block; ++n)
    {
        processChecked(p, audio, midi);
        if (firstAudibleBlock < 0 && audio.getMagnitude(0, block) > 1e-4f)
            firstAudibleBlock = n;
    }
    require(firstAudibleBlock >= 0 && e.hasHeldMidiNotes(), "immediate post-reset note did not sound");
    midi.addEvent(juce::MidiMessage::noteOff(1, 72), 0);
    float finalPeak = 0;
    const int releaseBlocks = (rate + block - 1) / block;
    for (int n = 0; n < releaseBlocks; ++n)
    {
        processChecked(p, audio, midi);
        if (n >= releaseBlocks / 2)
            finalPeak = std::max(finalPeak, audio.getMagnitude(0, block));
    }
    require(!e.hasHeldMidiNotes(), "fresh Note Off left ownership after reset");
    require(finalPeak < 1e-5f, "fresh Note Off left audible sound after fast release");
    unchanged(before, capture(p));
    std::cout << "PASS: fresh note releases without another reset or pedal-off at "
              << rate << '/' << block << "; first audible block=" << firstAudibleBlock
              << " (block-start audio timeline ms="
              << (1000.0 * firstAudibleBlock * block / rate) << ")\n";
}

static void testContentionAndDeferred(const juce::File& rom)
{
    auto owner = std::make_unique<VDX7AudioProcessor>(false);
    auto& p = *owner;
    initialise(p, rom, 48000, 64);
    const auto before = capture(p);
    juce::AudioBuffer<float> audio(2, 64);
    juce::MidiBuffer midi;
    {
        std::unique_lock lock(VDX7RegressionAccess::mutex(p));
        auto request = std::async(std::launch::async, [&] { resetChecked(p); });
        const bool timely = request.wait_for(std::chrono::seconds(1)) == std::future_status::ready;
        if (!timely) { lock.unlock(); request.get(); }
        require(timely, "reset waits for the engine mutex");
        request.get();
    }
    for (int n = 0; n < 1000; ++n) processChecked(p, audio, midi);
    // Put a stale selection and note behind a genuinely contended engine.
    {
        std::unique_lock lock(VDX7RegressionAccess::mutex(p));
        auto callback = std::async(std::launch::async, [&] {
            midi.addEvent(juce::MidiMessage::programChange(1, 7), 0);
            midi.addEvent(juce::MidiMessage::noteOn(1, 62, juce::uint8(100)), 2);
            processChecked(p, audio, midi);
        });
        const bool timely = callback.wait_for(std::chrono::seconds(1)) == std::future_status::ready;
        if (!timely) { lock.unlock(); callback.get(); }
        require(timely, "contended callback blocks");
        callback.get();
    }
    require(VDX7RegressionAccess::deferred(p), "stale MIDI fixture was not deferred");
    resetChecked(p);
    // This input is supplied after reset returns and must survive the normal
    // finite drain, unlike the old deferred program change and note above.
    midi.addEvent(juce::MidiMessage::noteOn(1, 69, juce::uint8(100)), 0);
    float peak = 0;
    for (int n = 0; n < 12000; ++n)
    { processChecked(p, audio, midi); peak = std::max(peak, audio.getMagnitude(0, 64)); }
    auto& e = VDX7RegressionAccess::engine(p);
    require(e.currentProgram() == before.program, "pre-reset deferred Program Change replayed");
    require(peak > 1e-4f && e.hasHeldMidiNotes(), "fresh post-reset MIDI was dropped");
    unchanged(before, capture(p));
    resetChecked(p);
    for (int n = 0; n < 12000; ++n)
    {
        processChecked(p, audio, midi);
        require(audio.getMagnitude(0, 64) < 1e-5f, "old/fresh voice reappeared after the second reset");
    }
    std::cout << "PASS: nonblocking reset, stale deferred input dropped, new host input retained\n";
}

int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    try
    {
        if (argc != 2 || !juce::File(argv[1]).existsAsFile())
            throw std::runtime_error("Supply an explicit compatible local ROM path");
        auto noRomOwner = std::make_unique<VDX7AudioProcessor>(false);
        auto& noRom = *noRomOwner;
        resetChecked(noRom);
        juce::AudioBuffer<float> silent(2, 64);
        juce::MidiBuffer empty;
        processChecked(noRom, silent, empty);
        require(silent.getMagnitude(0, 64) == 0, "no-ROM reset output");
        for (int rate : {44100, 48000, 96000})
            for (int block : {64, 256})
            {
                testHeldAndTail(juce::File(argv[1]), rate, block);
                testFreshNoteRelease(juce::File(argv[1]), rate, block);
            }
        testHeldAndTail(juce::File(argv[1]), 48000, 64, true);
        testContentionAndDeferred(juce::File(argv[1]));
    }
    catch (const std::exception& e)
    {
        VDX7AllocationProbe::enabled = false;
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
