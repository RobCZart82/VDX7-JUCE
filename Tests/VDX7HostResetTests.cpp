// Public host-reset and stopped-device lifecycle regressions.
// Execute with the user's explicitly supplied local ROM only.
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
    struct FirmwareOwnership
    {
        int midi = 0, held = 0, sustained = 0;
    };
    // Read-only diagnostic for the documented v1.8 RAM map. This is not a
    // production retirement predicate or proof that every ROM shares this map.
    static FirmwareOwnership firmwareOwnership(const VDX7AudioProcessor& p)
    {
        FirmwareOwnership result;
        const auto& memory = p.engine_.dx7_.memory;
        for (int i = 0; i < 16; ++i)
        {
            result.midi += (memory[0x2168 + i] & 0x80) != 0;
            result.held += (memory[0x20b1 + 2 * i] & 2) != 0;
            result.sustained += (memory[0x20b1 + 2 * i] & 1) != 0;
        }
        return result;
    }
    static int firmwareMidiOwnershipFor(const VDX7AudioProcessor& p, int note)
    {
        int count = 0;
        for (int i = 0; i < 16; ++i)
            count += p.engine_.dx7_.memory[0x2168 + i] == (0x80 | note);
        return count;
    }
    static unsigned pendingStages(const VDX7AudioProcessor& p)
    {
        const auto& d = p.engine_.dx7_;
        return (d.midiSerialRx.readIdx != d.midiSerialRx.writeIdx ? 1u : 0u)
            | ((d.TRCSR & (1u << dx7Emu::HD6303R::RDRF)) != 0 ? 2u : 0u)
            | (d.memory[0xee] != d.memory[0xf0] || d.memory[0xef] != d.memory[0xf1]
                || d.memory[0xf6] != 0 ? 4u : 0u);
    }
    static VDX7Engine& engine(VDX7AudioProcessor& p) { return p.engine_; }
    static int monoActiveCount(const VDX7AudioProcessor& p)
    { return p.engine_.dx7_.memory[0x8e]; }
    static void keepConservativeHistory(VDX7AudioProcessor& p)
    { p.engine_.releaseRetirementProfile_ = false; }
    static bool knownRetirementProfile(const VDX7AudioProcessor& p)
    { return p.engine_.releaseRetirementProfile_; }
    static bool knownFirmwareImage(const juce::MemoryBlock& image)
    {
        return (image.getSize() == VDX7Engine::kFirmwareSize
                || image.getSize() == VDX7Engine::kCombinedRomSize)
            && VDX7Engine::isReleaseRetirementFirmware(
                static_cast<const uint8_t*>(image.getData()), VDX7Engine::kFirmwareSize);
    }
    static void startShortLifecycleDrain(VDX7AudioProcessor& p)
    {
        // Force the same lifecycle drain to exhaust a one-block budget. This
        // is a shared-engine fallback test, not a public prepare/release call.
        p.engine_.beginMidiReset(true);
        p.engine_.advanceHostReset(64);
        p.engine_.prepare(48000);
    }
    static int releaseBudget(const VDX7AudioProcessor& p, int note)
    { return p.engine_.midiReleaseBudget_[note]; }
    static void checkFirmwareProfile(const VDX7AudioProcessor& p)
    {
        std::array<uint8_t, VDX7Engine::kFirmwareSize> copy{};
        std::copy_n(p.engine_.dx7_.memory + 0xc000, copy.size(), copy.begin());
        require(VDX7Engine::isReleaseRetirementFirmware(copy.data(), copy.size()), "known image not recognized");
        require(!VDX7Engine::isReleaseRetirementFirmware(nullptr, copy.size())
                && !VDX7Engine::isReleaseRetirementFirmware(copy.data(), copy.size() - 1), "invalid profile accepted");
        for (std::size_t i : {std::size_t(0), copy.size() / 2, copy.size() - 1})
        {
            copy[i] ^= 1;
            require(!VDX7Engine::isReleaseRetirementFirmware(copy.data(), copy.size()), "modified image accepted");
            copy[i] ^= 1;
        }
    }
    static std::mutex& mutex(VDX7AudioProcessor& p) { return p.engineMutex_; }
    static bool deferred(const VDX7AudioProcessor& p) { return p.deferredMidi_.active(); }
    static uint32_t dirty(const VDX7AudioProcessor& p) { return p.modifiedVoices_.load(); }
    static bool resetWaiting(const VDX7AudioProcessor& p)
    { return p.hostResetRequested_.load() || p.hostResetPending_; }
    static bool fullReleaseHistory(const VDX7AudioProcessor& p, int repeats)
    {
        return std::all_of(p.engine_.midiReleaseBudget_.begin(), p.engine_.midiReleaseBudget_.end(),
                           [repeats](uint8_t value) { return value == repeats; });
    }
    static bool inputIdle(VDX7AudioProcessor& p)
    {
        auto& e = p.engine_;
        auto& d = e.dx7_;
        return d.midiSerialRx.readIdx == d.midiSerialRx.writeIdx
            && (d.TRCSR & (1u << dx7Emu::HD6303R::RDRF)) == 0
            && d.memory[0xee] == d.memory[0xf0] && d.memory[0xef] == d.memory[0xf1]
            && d.memory[0xf6] == 0 && !d.haveMsg && e.appToSynth_.lfq.wasEmpty()
            && !p.deferredMidi_.active();
    }
    static std::vector<uint8_t> serialBytes(const VDX7AudioProcessor& p)
    {
        const auto& q = p.engine_.dx7_.midiSerialRx;
        std::vector<uint8_t> bytes;
        for (int i = q.readIdx; i != q.writeIdx; i = (i + 1) & (q.size - 1))
            bytes.push_back(q.buffer[i]);
        return bytes;
    }
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

static double processChecked(VDX7AudioProcessor& p, juce::AudioBuffer<float>& audio,
                           juce::MidiBuffer& midi)
{
    using namespace VDX7AllocationProbe;
    allocations = deallocations = 0;
    enabled = true;
    const auto start = std::chrono::steady_clock::now();
    p.processBlock(audio, midi);
    const auto finish = std::chrono::steady_clock::now();
    enabled = false;
    require(allocations == 0 && deallocations == 0, "reset callback allocates/deallocates ordinary C++ storage");
    for (int ch = 0; ch < audio.getNumChannels(); ++ch)
        for (int i = 0; i < audio.getNumSamples(); ++i)
            require(std::isfinite(audio.getSample(ch, i)), "non-finite reset output");
    require(midi.isEmpty(), "instrument must consume MIDI");
    return std::chrono::duration<double, std::micro>(finish - start).count();
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

static void testResetGateOverflow(const juce::File& rom, int releaseRate, int releaseFloor)
{
    auto owner = std::make_unique<VDX7AudioProcessor>(false);
    auto& p = *owner;
    initialise(p, rom, 48000, 64);
    auto& e = VDX7RegressionAccess::engine(p);
    e.setOperatorParameter(0, VDX7VoiceData::Parameter::rate4, releaseRate);
    e.setOperatorParameter(0, VDX7VoiceData::Parameter::level4, releaseFloor);
    e.reloadCurrentProgram();
    p.synchroniseOperatorParametersFromEngine();
    juce::AudioBuffer<float> audio(2, 64), zero(2, 0);
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 60, juce::uint8(100)), 0);
    float oldPeak = 0;
    for (int i = 0; i < 375; ++i)
    {
        processChecked(p, audio, midi);
        oldPeak = std::max(oldPeak, audio.getMagnitude(0, 64));
    }
    require(oldPeak > 1e-4f, "reset gate fixture must sound before reset");
    const auto before = capture(p);
    resetChecked(p);
    for (int i = 0; i < 750; ++i)
    {
        processChecked(p, audio, midi);
        require(audio.getMagnitude(0, 64) < 1e-5f, "reset gate fixture did not become silent");
    }
    const auto cleared = VDX7RegressionAccess::firmwareOwnership(p);
    require(!e.isHostResetInProgress() && cleared.midi == 0 && cleared.held == 0
            && cleared.sustained == 0, "old ownership survived reset gate fixture");

    const auto overloads = e.midiOverloadCount();
    midi.addEvent(juce::MidiMessage::noteOn(1, 72, juce::uint8(100)), 0);
    for (int i = 0; i < 3000; ++i)
        midi.addEvent(juce::MidiMessage::pitchWheel(1, 8192), 0);
    // Observe the post-dispatch queue before any firmware instruction runs.
    // The first accepted Note On was flushed by this same-offset burst.
    processChecked(p, zero, midi);
    require(e.midiOverloadCount() == overloads + 1 && e.isMidiRecovering(),
            "reset gate burst did not trigger recovery");
    const auto queued = VDX7RegressionAccess::serialBytes(p);
    require(queued.size() >= 384 && queued.size() % 3 == 0,
            "reset gate recovery queue shape");
    for (std::size_t i = 0; i < queued.size(); i += 3)
        require((queued[i] & 0xf0) == 0x80 && queued[i + 2] == 0,
                "flushed fresh Note On remains in the recovery queue");
    float leakedPeak = 0;
    bool sawFreshOwnership = false;
    for (int i = 0; i < 1500; ++i)
    {
        processChecked(p, audio, midi);
        leakedPeak = std::max(leakedPeak, audio.getMagnitude(0, 64));
        sawFreshOwnership |= VDX7RegressionAccess::firmwareMidiOwnershipFor(p, 72) != 0;
    }
    const auto recovered = VDX7RegressionAccess::firmwareOwnership(p);
    require(!sawFreshOwnership && recovered.midi == 0 && recovered.held == 0
            && recovered.sustained == 0, "discarded fresh note reached firmware ownership");
    std::cout << "RESET GATE OVERFLOW: R4=" << releaseRate << ", L4=" << releaseFloor
              << ", dropped note ownership=" << sawFreshOwnership
              << ", leaked peak=" << leakedPeak << '\n';
    require(leakedPeak < 1e-5f, "overflow-discarded fresh note opened reset gate and leaked L4 audio");
    unchanged(before, capture(p));
    midi.addEvent(juce::MidiMessage::noteOn(1, 74, juce::uint8(100)), 0);
    float freshPeak = 0;
    for (int i = 0; i < 375; ++i)
    {
        processChecked(p, audio, midi);
        freshPeak = std::max(freshPeak, audio.getMagnitude(0, 64));
    }
    require(freshPeak > 1e-4f && VDX7RegressionAccess::firmwareMidiOwnershipFor(p, 74) == 1,
            "valid fresh note failed to open reset gate after recovery");
    midi.addEvent(juce::MidiMessage::noteOff(1, 74), 0);
    for (int i = 0; i < 375; ++i) processChecked(p, audio, midi);
    const auto released = VDX7RegressionAccess::firmwareOwnership(p);
    require(released.midi == 0 && released.held == 0 && released.sustained == 0
            && !e.hasHeldMidiNotes(), "valid fresh note failed to release after gate recovery");
    if (releaseRate == 99 && releaseFloor != 0)
        require(audio.getMagnitude(0, 64) > 1e-4f,
                "nonzero-L4 positive control must remain audible after a genuine note release");
    // Nonzero L4 is intentionally not a silence-after-Note-Off fixture.
    unchanged(before, capture(p));
}

static void testExpandedLifecycle(const juce::File& rom, int mode, int path)
{
    auto owner = std::make_unique<VDX7AudioProcessor>(false);
    auto& p = *owner;
    initialise(p, rom, 48000, 64);
    auto& e = VDX7RegressionAccess::engine(p);
    e.setOperatorParameter(0, VDX7VoiceData::Parameter::rate4, 99);
    e.reloadCurrentProgram();
    p.synchroniseOperatorParametersFromEngine();
    require(e.setPlaySetting(0, 0), "expanded lifecycle initial POLY fixture");
    // Exercise the conservative path on a real known ROM; this is not a claim
    // to have tested other ROM images. MONO also retains history naturally.
    VDX7RegressionAccess::keepConservativeHistory(p);
    juce::AudioBuffer<float> audio(2, 64);
    juce::MidiBuffer midi;
    const auto pump = [&](int count)
    { for (int i = 0; i < count; ++i) processChecked(p, audio, midi); };
    const auto overloads = e.midiOverloadCount();
    for (int note = 0; note < 128; ++note)
    {
        // v1.8 MONO uses key zero as an empty-slot sentinel and does not
        // release it normally. Seed pitch-zero history in POLY, then exercise
        // all 127 other pitches in MONO. Keep the full 128*16 adapter budget;
        // do not confuse that separate native-firmware edge with lifecycle.
        if (note == 1) require(e.setPlaySetting(0, mode), "expanded lifecycle mode fixture");
        for (bool on : {true, false})
            for (int i = 0; i < 16; ++i)
            {
                midi.addEvent(on ? juce::MidiMessage::noteOn(1, note, juce::uint8(100))
                                 : juce::MidiMessage::noteOff(1, note), 0);
                pump(4);
            }
    }
    pump(375);
    require(VDX7RegressionAccess::fullReleaseHistory(p, 16), "expanded lifecycle history fixture");
    const auto idle = VDX7RegressionAccess::firmwareOwnership(p);
    require(idle.midi == 0 && idle.held == 0 && idle.sustained == 0
            && (mode == 0 || VDX7RegressionAccess::monoActiveCount(p) == 0),
            "expanded lifecycle history must finish with no firmware ownership");
    // Place held ownership near the END of the release batch (6144 bytes in
    // the old explicit-status path; 4097 with running status).
    for (int i = 0; i < 16; ++i)
        midi.addEvent(juce::MidiMessage::noteOn(1, 127, juce::uint8(100)), 0);
    pump(375);
    std::cout << "EXPANDED LIFECYCLE before: mode=" << mode
              << ", peak=" << audio.getMagnitude(0, 64) << '\n';
    require(audio.getMagnitude(0, 64) > 1e-4f, "expanded lifecycle held fixture must sound");
    require(VDX7RegressionAccess::firmwareMidiOwnershipFor(p, 127) == 16
            && e.midiOverloadCount() == overloads, "expanded lifecycle held ownership fixture");
    const auto before = capture(p);
    const auto started = std::chrono::steady_clock::now();
    if (path == 2)
        VDX7RegressionAccess::startShortLifecycleDrain(p);
    else
    {
        if (path == 1) p.releaseResources();
        p.prepareToPlay(48000, 64);
    }
    const auto elapsed = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - started).count();
    const auto afterLifecycle = VDX7RegressionAccess::firmwareOwnership(p);
    std::cout << "EXPANDED LIFECYCLE: mode=" << mode << ", path=" << path
              << ", old MIDI ownership immediately=" << afterLifecycle.midi
              << ", lifecycle wall ms=" << elapsed << '\n';
    if (path == 2)
        require(e.isHostResetInProgress() && afterLifecycle.midi == 16,
                "short lifecycle budget must leave a pending drain, not fake completion");
    midi.addEvent(juce::MidiMessage::noteOn(1, 72, juce::uint8(100)), 0);
    float peak = 0;
    int firstAudibleBlock = -1;
    for (int i = 0; i < 1500; ++i)
    {
        processChecked(p, audio, midi);
        peak = std::max(peak, audio.getMagnitude(0, 64));
        if (firstAudibleBlock < 0 && audio.getMagnitude(0, 64) > 1e-4f) firstAudibleBlock = i;
    }
    const auto fresh = VDX7RegressionAccess::firmwareOwnership(p);
    std::cout << "EXPANDED LIFECYCLE fresh: peak=" << peak
              << ", note72=" << VDX7RegressionAccess::firmwareMidiOwnershipFor(p, 72)
              << ", old127=" << VDX7RegressionAccess::firmwareMidiOwnershipFor(p, 127)
              << ", MIDI/held/sustained=" << fresh.midi << '/' << fresh.held
              << '/' << fresh.sustained << ", onset ms=" << (firstAudibleBlock + 1) * 64.0 / 48.0 << '\n';
    require(peak > 1e-4f && VDX7RegressionAccess::firmwareMidiOwnershipFor(p, 72) == 1
            && VDX7RegressionAccess::firmwareMidiOwnershipFor(p, 127) == 0
            && fresh.midi == 1 && fresh.held == 1 && fresh.sustained == 0
            && !e.isHostResetInProgress() && e.midiOverloadCount() == overloads,
            "expanded lifecycle retained old ownership or lost immediate fresh note");
    midi.addEvent(juce::MidiMessage::noteOff(1, 72), 0);
    // A short non-RT budget leaves ~1.5 seconds on the existing deferred
    // timeline. Its Note Off has the same delay as its Note On; allow the
    // original two-second window, not a one-second test-only deadline.
    pump(1500);
    const auto released = VDX7RegressionAccess::firmwareOwnership(p);
    require(released.midi == 0 && released.held == 0 && released.sustained == 0
            && audio.getMagnitude(0, 64) < 1e-5f, "expanded lifecycle fresh note failed to release");
    unchanged(before, capture(p));
}

static void diagnoseMonoNoteZero(const juce::File& rom)
{
    auto owner = std::make_unique<VDX7AudioProcessor>(false);
    auto& p = *owner;
    initialise(p, rom, 48000, 64);
    require(VDX7RegressionAccess::engine(p).setPlaySetting(0, 1), "MONO diagnostic fixture");
    juce::AudioBuffer<float> audio(2, 64);
    juce::MidiBuffer midi;
    for (bool on : {true, false})
    {
        for (int i = 0; i < 16; ++i)
            midi.addEvent(on ? juce::MidiMessage::noteOn(1, 0, juce::uint8(100))
                             : juce::MidiMessage::noteOff(1, 0), 0);
        for (int i = 0; i < 375; ++i) processChecked(p, audio, midi);
    }
    const auto fw = VDX7RegressionAccess::firmwareOwnership(p);
    std::cout << "MONO pitch-zero without any reset: MIDI=" << fw.midi
              << ", held=" << fw.held << ", mono count="
              << VDX7RegressionAccess::monoActiveCount(p) << '\n';
    require(fw.midi == 0 && fw.held == 0 && VDX7RegressionAccess::monoActiveCount(p) == 0,
            "MONO pitch-zero native release edge (separate from lifecycle drain)");
}

// Independent of VDX7Engine/processor MIDI, reset, retirement and resampling.
// Only the pinned unmodified dx7Lib core executes the user's original firmware.
class BareFirmware
{
public:
    explicit BareFirmware(const juce::File& rom) : machine(toSynth, toGui)
    {
        require(rom.loadFileAsData(image), "raw-core ROM read");
        require(VDX7RegressionAccess::knownFirmwareImage(image), "raw-core known ROM fixture");
        auto* bytes = static_cast<const uint8_t*>(image.getData());
        require(machine.loadFirmware(bytes, VDX7Engine::kFirmwareSize), "raw-core firmware load");
        if (image.getSize() == VDX7Engine::kCombinedRomSize)
            require(machine.loadVoices(bytes + VDX7Engine::kFirmwareSize,
                                       VDX7Engine::kFactoryVoicesSize), "raw-core factory load");
        else
            require(machine.loadVoices(blankBank.data(), blankBank.size()), "raw-core blank bank");
        machine.start();
        for (int i = 0; i < 3000000; ++i) machine.run();
        machine.initControllers();
        dx7Emu::Message discarded;
        while (queue.pop(discarded)) {}
        machine.sustain(false);
        machine.porta(false);
        machine.setBank(0, false);
        send(0xc0, 3);
        pump();
    }

    void setMode(int mode)
    {
        send(0xb0, mode ? 126 : 127, mode ? 1 : 0);
        pump();
        require(machine.memory[0x20a9] == mode, "raw-core mode change");
    }
    void notes(int note, int repeats, bool on, bool zeroVelocityOff)
    {
        for (int i = 0; i < repeats; ++i)
            send(on || zeroVelocityOff ? 0x90 : 0x80, note, on ? 100 : 0);
        pump();
    }
    // Counts are read only after all staged input has drained, not used to
    // mutate firmware state or to tell the production reset when to finish.
    std::array<int, 4> counts() const
    {
        std::array<int, 4> result {0, 0, 0, machine.memory[0x8e]};
        for (int i = 0; i < 16; ++i)
        {
            result[0] += (machine.memory[0x2168 + i] & 0x80) != 0;
            result[1] += (machine.memory[0x20b1 + 2 * i] & 2) != 0;
            result[2] += (machine.memory[0x20b1 + 2 * i] & 1) != 0;
        }
        return result;
    }
private:
    void send(int status, int data1, int data2 = -1)
    {
        machine.midiSerialRx.write(static_cast<uint8_t>(status | (machine.getMidiRxChannel() & 15)));
        machine.midiSerialRx.write(static_cast<uint8_t>(data1));
        if (data2 >= 0) machine.midiSerialRx.write(static_cast<uint8_t>(data2));
    }
    void pump()
    {
        for (int samples = 0; samples < 24000;)
        {
            machine.run();
            std::array<float, 16> discarded {};
            int emitted = 0;
            const int cycles = machine.inst != nullptr && machine.inst->cycles > 0
                ? machine.inst->cycles : 1;
            machine.egs.clock(discarded.data(), emitted, cycles * 4);
            samples += emitted;
        }
        require(machine.midiSerialRx.empty()
                && (machine.TRCSR & (1u << dx7Emu::HD6303R::RDRF)) == 0
                && machine.memory[0xee] == machine.memory[0xf0]
                && machine.memory[0xef] == machine.memory[0xf1]
                && machine.memory[0xe7] == 0 && machine.memory[0xf6] == 0,
                "raw-core input still pending");
    }
    dx7Emu::App_ToSynth queue;
    dx7Emu::NullToGui gui;
    dx7Emu::ToSynth* toSynth = &queue;
    dx7Emu::ToGui* toGui = &gui;
    juce::MemoryBlock image;
    std::array<uint8_t, 4096> blankBank {};
    dx7Emu::DX7 machine;
};

static void characterizeMonoBoundary(const juce::File& rom, int mode, int note,
                                     int repeats, bool zeroVelocityOff)
{
    auto raw = std::make_unique<BareFirmware>(rom);
    raw->setMode(mode);
    require(raw->counts() == std::array<int, 4>{}, "raw-core empty starting ownership");
    auto owner = std::make_unique<VDX7AudioProcessor>(false);
    auto& p = *owner;
    initialise(p, rom, 48000, 64);
    auto& e = VDX7RegressionAccess::engine(p);
    e.setOperatorParameter(0, VDX7VoiceData::Parameter::rate4, 99);
    e.reloadCurrentProgram();
    p.synchroniseOperatorParametersFromEngine();
    require(e.setPlaySetting(0, mode), "boundary mode fixture");
    const auto before = capture(p);
    juce::AudioBuffer<float> audio(2, 64);
    juce::MidiBuffer midi;
    const auto pump = [&]
    { for (int i = 0; i < 375; ++i) processChecked(p, audio, midi); };
    for (bool on : {true, false})
    {
        raw->notes(note, repeats, on, zeroVelocityOff);
        for (int i = 0; i < repeats; ++i)
            midi.addEvent(on || zeroVelocityOff
                ? juce::MidiMessage::noteOn(1, note, juce::uint8(on ? 100 : 0))
                : juce::MidiMessage::noteOff(1, note), 0);
        pump();
        const auto fw = VDX7RegressionAccess::firmwareOwnership(p);
        const std::array<int, 4> actual {fw.midi, fw.held, fw.sustained,
                                       VDX7RegressionAccess::monoActiveCount(p)};
        const bool nativeZeroEdge = mode == 1 && note == 0;
        const std::array<int, 4> expected {
            on ? repeats : 0,
            nativeZeroEdge ? 1 : on ? repeats : 0,
            0,
            mode == 1 && (on || nativeZeroEdge) ? repeats : 0};
        require(raw->counts() == expected, "raw-core boundary differs from explicit native expectation");
        require(actual == expected, "processor boundary differs from explicit native expectation");
    }
    unchanged(before, capture(p));
    std::cout << "CHARACTERIZATION (not a fix): mode=" << mode << ", pitch=" << note
              << ", repeats=" << repeats << ", velocity-zero-off=" << zeroVelocityOff
              << ", raw/processor match; native zero edge=" << (mode == 1 && note == 0) << '\n';

    // Prove a firmware-driven recovery route, without patching its RAM or
    // introducing an automatic mode change into ordinary plugin playback.
    if (mode == 1 && note == 0)
    {
        raw->setMode(0);
        raw->setMode(1);
        require(raw->counts() == std::array<int, 4>{}, "raw native mode-cycle recovery");
        require(e.setPlaySetting(0, 0) && e.setPlaySetting(0, 1), "processor native mode-cycle recovery");
    }
    midi.addEvent(juce::MidiMessage::noteOn(1, 72, juce::uint8(100)), 0);
    float peak = 0;
    for (int i = 0; i < 375; ++i)
    {
        processChecked(p, audio, midi);
        peak = std::max(peak, audio.getMagnitude(0, 64));
    }
    const auto fresh = VDX7RegressionAccess::firmwareOwnership(p);
    require(peak > 1e-4f && VDX7RegressionAccess::firmwareMidiOwnershipFor(p, 72) == 1
            && fresh.midi == 1 && fresh.held == 1 && fresh.sustained == 0,
            "boundary control/recovery fresh note");
    midi.addEvent(juce::MidiMessage::noteOff(1, 72), 0);
    pump();
    const auto released = VDX7RegressionAccess::firmwareOwnership(p);
    require(released.midi == 0 && released.held == 0 && released.sustained == 0
            && VDX7RegressionAccess::monoActiveCount(p) == 0
            && audio.getMagnitude(0, 64) < 1e-5f, "boundary control/recovery fresh release");
    unchanged(before, capture(p));
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

static void testReactivation(const juce::File& rom, bool observeRequest, bool releaseFirst = true)
{
    auto owner = std::make_unique<VDX7AudioProcessor>(false);
    auto& p = *owner;
    initialise(p, rom, 48000, 64);
    juce::AudioBuffer<float> audio(2, 64);
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 60, juce::uint8(100)), 0);
    for (int n = 0; n < 100; ++n) processChecked(p, audio, midi);
    require(VDX7RegressionAccess::engine(p).hasHeldMidiNotes(), "reactivation fixture needs a note");
    const auto before = capture(p);
    resetChecked(p);
    if (observeRequest)
    {
        // Observe the atomic request without allowing the engine lock. This
        // leaves the audio-owned pending flag for the lifecycle to retire.
        std::unique_lock lock(VDX7RegressionAccess::mutex(p));
        auto callback = std::async(std::launch::async, [&] { processChecked(p, audio, midi); });
        const bool timely = callback.wait_for(std::chrono::seconds(1)) == std::future_status::ready;
        if (!timely) { lock.unlock(); callback.get(); }
        require(timely, "reactivation fixture callback blocked on engine mutex");
        callback.get();
    }
    require(VDX7RegressionAccess::resetWaiting(p), "fixture did not retain reset request");
    if (releaseFirst)
    {
        p.releaseResources();
        require(!VDX7RegressionAccess::resetWaiting(p), "release did not retire old reset");
    }
    p.prepareToPlay(48000, 64);
    require(!VDX7RegressionAccess::resetWaiting(p), "stale host reset survived release/prepare");
    unchanged(before, capture(p));
    midi.addEvent(juce::MidiMessage::noteOn(1, 72, juce::uint8(100)), 0);
    float peak = 0;
    for (int n = 0; n < 375; ++n)
    {
        processChecked(p, audio, midi);
        require(!VDX7RegressionAccess::engine(p).isHostResetInProgress(),
                "reactivation started a redundant host reset");
        peak = std::max(peak, audio.getMagnitude(0, 64));
    }
    require(peak > 1e-4f, "fresh reactivation note did not sound");
    midi.addEvent(juce::MidiMessage::noteOff(1, 72), 0);
    processChecked(p, audio, midi);
    require(!VDX7RegressionAccess::engine(p).hasHeldMidiNotes(), "reactivation note did not release");
    // A genuinely new request after prepare must still be observed.
    resetChecked(p);
    require(VDX7RegressionAccess::resetWaiting(p), "new post-prepare reset was lost");
    for (int n = 0; n < 750; ++n)
    {
        processChecked(p, audio, midi);
        require(audio.getMagnitude(0, 64) < 1e-5f, "new reset did not silence reactivation tail");
    }
    std::cout << "PASS: " << (releaseFirst ? "release/prepare" : "prepare alone")
              << " retires " << (observeRequest ? "pending" : "unobserved")
              << " reset; fresh input and later reset work\n";
}

static void testRunningStatusRelease(const juce::File& rom, bool compact, int repeats)
{
    auto owner = std::make_unique<VDX7AudioProcessor>(false);
    auto& p = *owner;
    initialise(p, rom, 48000, 64);
    auto& e = VDX7RegressionAccess::engine(p);
    e.setOperatorParameter(0, VDX7VoiceData::Parameter::rate4, 99);
    e.reloadCurrentProgram();
    p.synchroniseOperatorParametersFromEngine();
    juce::AudioBuffer<float> audio(2, 64);
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 60, juce::uint8(100)), 0);
    for (int n = 0; n < repeats; ++n)
        midi.addEvent(juce::MidiMessage::noteOn(1, 72, juce::uint8(100)), 0);
    float peak = 0;
    for (int n = 0; n < 375; ++n)
    { processChecked(p, audio, midi); peak = std::max(peak, audio.getMagnitude(0, 64)); }
    require(peak > 1e-4f, "running-status fixture must sound");
    // No host reset, output mute gate or EGS reconstruction: only the real
    // firmware's interpretation of the serial Note Off batch can stop this.
    e.allNotesOff(compact);
    float tail = 0;
    for (int n = 0; n < 750; ++n)
    {
        processChecked(p, audio, midi);
        if (n >= 375) tail = std::max(tail, audio.getMagnitude(0, 64));
    }
    require(tail < 1e-5f && !e.hasHeldMidiNotes(), "firmware did not release serial batch");
    std::cout << "PASS: firmware serial release, compact=" << compact
              << ", repeated notes=" << repeats << '\n';
}

static SavedSettings testExpandedHistory(const juce::File& rom, int repeats, int rate = 48000,
                                        int block = 64, bool measurePair = false,
                                        bool conservative = true, std::array<int, 2>* timing = nullptr)
{
    auto owner = std::make_unique<VDX7AudioProcessor>(false);
    auto& p = *owner;
    initialise(p, rom, rate, block);
    // Preserve the original maximum-history regression for the fallback path.
    if (conservative) VDX7RegressionAccess::keepConservativeHistory(p);
    else
    {
        require(VDX7RegressionAccess::knownRetirementProfile(p), "retirement fixture needs validated ROM");
        VDX7RegressionAccess::checkFirmwareProfile(p);
    }
    auto& e = VDX7RegressionAccess::engine(p);
    e.setOperatorParameter(0, VDX7VoiceData::Parameter::rate4, 99);
    e.reloadCurrentProgram();
    p.synchroniseOperatorParametersFromEngine();
    juce::AudioBuffer<float> audio(2, block);
    juce::MidiBuffer midi;
    const auto overloads = e.midiOverloadCount();
    // Public MIDI input, not a fabricated private budget. Pace each event so
    // this fixture grows lifetime ownership history without overflowing input.
    for (int note = 0; note < 128; ++note)
    {
        for (bool on : {true, false})
            for (int repeat = 0; repeat < repeats; ++repeat)
            {
                midi.addEvent(on ? juce::MidiMessage::noteOn(1, note, juce::uint8(100))
                                 : juce::MidiMessage::noteOff(1, note), 0);
                for (int n = 0; n < 4; ++n) processChecked(p, audio, midi);
            }
    }
    require(!e.hasHeldMidiNotes(), "history fixture left held notes");
    require(e.midiOverloadCount() == overloads, "history fixture overflowed input");
    if (conservative)
        require(VDX7RegressionAccess::fullReleaseHistory(p, repeats), "history fixture did not reach requested budget");
    if (measurePair)
    {
        // Match observable idle input/ownership and silence, not arbitrary CPU
        // RAM, oscillator phases or undocumented firmware voice-slot contents.
        for (int n = 0; n < rate / block; ++n) processChecked(p, audio, midi);
        require(VDX7RegressionAccess::inputIdle(p), "paired fixture has pending input");
        require(audio.getMagnitude(0, block) < 1e-5f, "paired fixture has audible tail");
        const auto ownership = VDX7RegressionAccess::firmwareOwnership(p);
        std::cout << "FIRMWARE pair: MIDI=" << ownership.midi << ", held=" << ownership.held
                  << ", sustained=" << ownership.sustained << '\n';
        require(ownership.midi == 0 && ownership.held == 0 && ownership.sustained == 0,
                "paired fixture retains firmware ownership");
        if (!conservative)
            require(VDX7RegressionAccess::fullReleaseHistory(p, 0), "completed history was not retired during normal playback");
    }
    const auto before = capture(p);
    resetChecked(p);
    double maxCallbackUs = 0;
    if (measurePair)
    {
        juce::AudioBuffer<float> zero(2, 0);
        const auto setupUs = processChecked(p, zero, midi);
        // Observe the actual queued bytes before any audio-time drain. This
        // zero-sample observation is separate from the immediate-input matrix.
        const auto bytes = VDX7RegressionAccess::serialBytes(p);
        const size_t releases = conservative ? static_cast<size_t>(128 * repeats) : 0;
        require(bytes.size() == (releases == 0 ? 0 : 1 + 2 * releases), "unexpected reset serial byte count");
        if (!bytes.empty()) require(bytes[0] == 0x80, "unexpected reset status");
        size_t decoded = 0;
        for (size_t i = 1; i + 1 < bytes.size(); i += 2)
        {
            require(bytes[i] == decoded / repeats && bytes[i + 1] == 0, "unexpected reset Note Off payload");
            ++decoded;
        }
        require(decoded == releases, "unexpected decoded reset release count");
        std::cout << "PAIR: history=" << repeats << ", decoded Note Off=" << decoded
                  << ", queued serial bytes=" << bytes.size() << ", setup callback us=" << setupUs << '\n';
    }
    midi.addEvent(juce::MidiMessage::noteOn(1, 72, juce::uint8(100)), 0);
    int drainBlock = -1, audibleBlock = -1;
    for (int n = 0; n < rate * 10 / block; ++n)
    {
        maxCallbackUs = std::max(maxCallbackUs, processChecked(p, audio, midi));
        if (drainBlock < 0 && !e.isHostResetInProgress()) drainBlock = n;
        if (audibleBlock < 0 && audio.getMagnitude(0, block) > 1e-4f) audibleBlock = n;
    }
    std::cout << "HISTORY: " << rate << '/' << block << ", 128 pitches x" << repeats
              << "; reset completion block=" << drainBlock
              << " (block-end ms=" << (1000.0 * (drainBlock + 1) * block / rate)
              << "); first audible block=" << audibleBlock
              << " (block-end ms=" << (1000.0 * (audibleBlock + 1) * block / rate)
              << "); max observed callback us=" << maxCallbackUs << std::endl;
    require(drainBlock >= 0, "expanded-history reset did not complete in observation window");
    if (timing != nullptr) *timing = {drainBlock, audibleBlock};
    require(audibleBlock >= 0 && e.hasHeldMidiNotes(), "expanded-history reset lost fresh note");
    midi.addEvent(juce::MidiMessage::noteOff(1, 72), 0);
    float finalPeak = 0;
    // The existing queue shifts subsequent Note Off by the same reset delay.
    // Observe that measured delay plus a second; do not mistake delayed release
    // for lost release. The large onset/release latency is reported separately.
    const int releaseBlocks = drainBlock + 1 + rate / block;
    for (int n = 0; n < releaseBlocks; ++n)
    {
        processChecked(p, audio, midi);
        if (n >= releaseBlocks - rate / block / 2)
            finalPeak = std::max(finalPeak, audio.getMagnitude(0, block));
    }
    require(!e.hasHeldMidiNotes() && finalPeak < 1e-5f, "history fresh note did not release");
    unchanged(before, capture(p));
    return before;
}

static void testFirmwareOwnership(const juce::File& rom, int note, int repeats, bool sustain)
{
    auto owner = std::make_unique<VDX7AudioProcessor>(false);
    auto& p = *owner;
    initialise(p, rom, 48000, 64);
    auto& e = VDX7RegressionAccess::engine(p);
    juce::AudioBuffer<float> audio(2, 64);
    juce::MidiBuffer midi;
    const auto send = [&e](const juce::MidiMessage& message)
    { e.handleMidi(message.getRawData(), message.getRawDataSize()); };
    const auto settle = [&]
    { for (int i = 0; i < 375; ++i) processChecked(p, audio, midi); };
    const auto check = [&](int expectedMidi, int expectedHeld, int expectedSustained)
    {
        const auto s = VDX7RegressionAccess::firmwareOwnership(p);
        require(s.midi == expectedMidi && s.held == expectedHeld && s.sustained == expectedSustained,
                "firmware ownership transition differs from documented map");
    };
    check(0, 0, 0);
    if (sustain) send(juce::MidiMessage::controllerEvent(1, 64, 127));
    for (int i = 0; i < repeats; ++i)
        send(juce::MidiMessage::noteOn(1, note, juce::uint8(100)));
    settle();
    check(repeats, repeats, 0);
    require(VDX7RegressionAccess::releaseBudget(p, note) == repeats, "held-note history retired prematurely");
    require(VDX7RegressionAccess::inputIdle(p), "held fixture input did not settle");
    for (int i = 0; i < repeats; ++i) send(juce::MidiMessage::noteOff(1, note));
    // The adapter has already decremented every note; the firmware still owns
    // all voices until the queued releases actually execute. Never retire here.
    check(repeats, repeats, 0);
    require(!VDX7RegressionAccess::inputIdle(p), "queued release was not observable");
    require(VDX7RegressionAccess::releaseBudget(p, note) == repeats, "queued-release history retired prematurely");
    if (!sustain) require(!e.hasHeldMidiNotes(), "adapter did not accept releases");
    unsigned stages = VDX7RegressionAccess::pendingStages(p);
    juce::AudioBuffer<float> single(2, 1);
    for (int i = 0; i < 12000; ++i)
    {
        processChecked(p, single, midi);
        stages |= VDX7RegressionAccess::pendingStages(p);
    }
    require(VDX7RegressionAccess::inputIdle(p), "released fixture input did not settle");
    require(stages == 7, "release was not observed at every input stage");
    check(0, 0, sustain ? repeats : 0);
    if (sustain)
    {
        require(VDX7RegressionAccess::releaseBudget(p, note) == repeats, "sustained history retired prematurely");
        // Empty MIDI ownership and empty queues do NOT mean all voices are off.
        send(juce::MidiMessage::controllerEvent(1, 64, 0));
        settle();
        check(0, 0, 0);
    }
    require(VDX7RegressionAccess::releaseBudget(p, note) == 0, "normal-playback release history not retired");
    // No reset, mute-gate opening/closing or EGS reconstruction in this test.
    require(!e.isHostResetInProgress(), "ownership test unexpectedly reset");
    std::cout << "PASS: firmware ownership transitions note=" << note << ", repeats=" << repeats
              << ", sustain=" << sustain << ", observed release stages=" << stages << '\n';
}

static void testResetAtReleaseStage(const juce::File& rom, unsigned stage, bool sustain)
{
    auto owner = std::make_unique<VDX7AudioProcessor>(false);
    auto& p = *owner;
    initialise(p, rom, 48000, 64);
    auto& e = VDX7RegressionAccess::engine(p);
    e.setOperatorParameter(0, VDX7VoiceData::Parameter::rate4, 99);
    e.reloadCurrentProgram();
    p.synchroniseOperatorParametersFromEngine();
    juce::AudioBuffer<float> audio(2, 64), single(2, 1);
    juce::MidiBuffer midi;
    const auto send = [&e](const juce::MidiMessage& message)
    { e.handleMidi(message.getRawData(), message.getRawDataSize()); };
    if (sustain) send(juce::MidiMessage::controllerEvent(1, 64, 127));
    for (int i = 0; i < 16; ++i) send(juce::MidiMessage::noteOn(1, 60, juce::uint8(100)));
    for (int i = 0; i < 375; ++i) processChecked(p, audio, midi);
    require(VDX7RegressionAccess::firmwareOwnership(p).midi == 16, "stage fixture needs 16 voices");
    for (int i = 0; i < 16; ++i) send(juce::MidiMessage::noteOff(1, 60));
    const auto atStage = [&]
    {
        const unsigned flags = VDX7RegressionAccess::pendingStages(p);
        // SCI after adapter drain; internal processing after adapter AND SCI
        // drain. Do not mislabel an early batch with all three stages pending.
        return (flags & stage) != 0 && (flags & (stage - 1)) == 0;
    };
    int steps = 0;
    while (!atStage() && steps++ < 12000)
        processChecked(p, single, midi);
    require(atStage(), "requested release stage not reached");
    require(VDX7RegressionAccess::releaseBudget(p, 60) == 16, "in-flight release history retired prematurely");
    const auto before = capture(p);
    resetChecked(p);
    midi.addEvent(juce::MidiMessage::noteOn(1, 72, juce::uint8(100)), 0);
    float peak = 0;
    for (int i = 0; i < 1500; ++i)
    {
        processChecked(p, audio, midi);
        peak = std::max(peak, audio.getMagnitude(0, 64));
    }
    const auto fresh = VDX7RegressionAccess::firmwareOwnership(p);
    require(peak > 1e-4f && fresh.midi == 1 && fresh.held == 1 && fresh.sustained == 0,
            "stage reset lost fresh ownership or retained old firmware voices");
    midi.addEvent(juce::MidiMessage::noteOff(1, 72), 0);
    float tail = 0;
    for (int i = 0; i < 1500; ++i)
    {
        processChecked(p, audio, midi);
        if (i >= 1125) tail = std::max(tail, audio.getMagnitude(0, 64));
    }
    const auto released = VDX7RegressionAccess::firmwareOwnership(p);
    require(tail < 1e-5f && released.midi == 0 && released.held == 0 && released.sustained == 0,
            "stage reset fresh voice did not release");
    unchanged(before, capture(p));
    std::cout << "PASS: reset at release stage=" << stage << ", sustain=" << sustain << '\n';
}

static std::array<int, 2> testOverlappingHistory(const juce::File& rom, int repeats,
                                               int rate, int block)
{
    auto owner = std::make_unique<VDX7AudioProcessor>(false);
    auto& p = *owner;
    initialise(p, rom, rate, block);
    auto& e = VDX7RegressionAccess::engine(p);
    e.setOperatorParameter(0, VDX7VoiceData::Parameter::rate4, 99);
    e.reloadCurrentProgram();
    p.synchroniseOperatorParametersFromEngine();
    require(VDX7RegressionAccess::knownRetirementProfile(p), "overlap fixture needs validated ROM");
    juce::AudioBuffer<float> audio(2, block);
    juce::MidiBuffer midi;
    const auto pump = [&](int count)
    { for (int i = 0; i < count; ++i) processChecked(p, audio, midi); };
    midi.addEvent(juce::MidiMessage::noteOn(1, 60, juce::uint8(100)), 0);
    pump(rate / block / 4);
    const auto overloads = e.midiOverloadCount();
    // Never release this anchor while playing/releasing every OTHER pitch.
    // The adapter and firmware therefore never have globally idle ownership.
    for (int note = 0; note < 128; ++note)
    {
        if (note == 60) continue;
        for (bool on : {true, false})
            for (int i = 0; i < repeats; ++i)
            {
                midi.addEvent(on ? juce::MidiMessage::noteOn(1, note, juce::uint8(100))
                                 : juce::MidiMessage::noteOff(1, note), 0);
                pump(4);
            }
        require(VDX7RegressionAccess::firmwareMidiOwnershipFor(p, 60) == 1,
                "overlap fixture lost the anchor");
    }
    pump(rate / block);
    require(e.midiOverloadCount() == overloads, "overlap history fixture overflowed");
    const auto owned = VDX7RegressionAccess::firmwareOwnership(p);
    require(owned.midi == 1 && owned.held == 1 && owned.sustained == 0,
            "overlap fixture has unexpected firmware ownership");
    for (int note = 0; note < 128; ++note)
        require(VDX7RegressionAccess::releaseBudget(p, note) == (note == 60 ? 1 : 0),
                "released neighboring pitches retain history while anchor is held");
    const auto before = capture(p);
    resetChecked(p);
    juce::AudioBuffer<float> zero(2, 0);
    processChecked(p, zero, midi);
    require(VDX7RegressionAccess::serialBytes(p) == std::vector<uint8_t>({0x80, 60, 0}),
            "overlap reset must release only the anchor");
    midi.addEvent(juce::MidiMessage::noteOn(1, 72, juce::uint8(100)), 0);
    int drain = -1, onset = -1;
    for (int i = 0; i < rate * 2 / block; ++i)
    {
        processChecked(p, audio, midi);
        if (drain < 0 && !e.isHostResetInProgress()) drain = i;
        if (onset < 0 && audio.getMagnitude(0, block) > 1e-4f) onset = i;
    }
    require(drain >= 0 && onset >= 0 && VDX7RegressionAccess::firmwareMidiOwnershipFor(p, 72) == 1
            && VDX7RegressionAccess::firmwareMidiOwnershipFor(p, 60) == 0,
            "overlap reset lost fresh pitch or retained the anchor");
    midi.addEvent(juce::MidiMessage::noteOff(1, 72), 0);
    pump(rate / block);
    const auto released = VDX7RegressionAccess::firmwareOwnership(p);
    require(released.midi == 0 && released.held == 0 && released.sustained == 0
            && audio.getMagnitude(0, block) < 1e-5f, "overlap fresh note failed to release");
    unchanged(before, capture(p));
    std::cout << "OVERLAP: " << rate << '/' << block << ", neighboring repeats=" << repeats
              << ", reset ms=" << 1000.0 * (drain + 1) * block / rate
              << ", onset ms=" << 1000.0 * (onset + 1) * block / rate << '\n';
    return {drain, onset};
}

static void testRetiredHistoryOverflow(const juce::File& rom, unsigned stage, bool sustain)
{
    auto owner = std::make_unique<VDX7AudioProcessor>(false);
    auto& p = *owner;
    initialise(p, rom, 48000, 64);
    auto& e = VDX7RegressionAccess::engine(p);
    e.setOperatorParameter(0, VDX7VoiceData::Parameter::rate4, 99);
    e.reloadCurrentProgram();
    p.synchroniseOperatorParametersFromEngine();
    juce::AudioBuffer<float> audio(2, 64), single(2, 1);
    juce::MidiBuffer midi;
    const auto send = [&e](const juce::MidiMessage& message)
    { e.handleMidi(message.getRawData(), message.getRawDataSize()); };
    const auto pump = [&](int count)
    {
        float tail = 0;
        for (int i = 0; i < count; ++i)
        {
            processChecked(p, audio, midi);
            if (i >= count / 2) tail = std::max(tail, audio.getMagnitude(0, 64));
        }
        return tail;
    };
    // Retire a full repeated neighboring pitch while keeping the anchor held.
    send(juce::MidiMessage::noteOn(1, 60, juce::uint8(100)));
    pump(100);
    for (bool on : {true, false})
        for (int i = 0; i < 16; ++i)
        {
            send(on ? juce::MidiMessage::noteOn(1, 62, juce::uint8(100))
                    : juce::MidiMessage::noteOff(1, 62));
            pump(4);
        }
    pump(100);
    require(VDX7RegressionAccess::releaseBudget(p, 62) == 0
            && VDX7RegressionAccess::releaseBudget(p, 60) == 1,
            "overflow fixture needs retired neighbor and protected anchor");
    if (sustain) { send(juce::MidiMessage::controllerEvent(1, 64, 127)); pump(100); }
    const auto before = capture(p);
    const auto overloads = e.midiOverloadCount();
    send(juce::MidiMessage::noteOff(1, 60));
    const auto atStage = [&]
    {
        const unsigned flags = VDX7RegressionAccess::pendingStages(p);
        return (flags & stage) != 0 && (flags & (stage - 1)) == 0;
    };
    int steps = 0;
    while (!atStage() && steps++ < 12000) processChecked(p, single, midi);
    require(atStage(), "overlap overflow release stage not reached");
    require(VDX7RegressionAccess::releaseBudget(p, 60) == 1,
            "pending anchor release retired before overflow");
    for (int i = 0; i < 3000; ++i) send(juce::MidiMessage::pitchWheel(1, 8192));
    require(e.midiOverloadCount() == overloads + 1 && e.isMidiRecovering()
            && VDX7RegressionAccess::releaseBudget(p, 60) == 1,
            "overflow did not preserve pending anchor release history");
    const float tail = pump(1500);
    const auto cleared = VDX7RegressionAccess::firmwareOwnership(p);
    require(cleared.midi == 0 && cleared.held == 0 && cleared.sustained == 0
            && !e.hasHeldMidiNotes() && !e.isMidiRecovering() && tail < 1e-5f,
            "overlap overflow retained ownership or audible stuck voice");
    send(juce::MidiMessage::noteOn(1, 72, juce::uint8(100)));
    require(pump(375) > 1e-4f && VDX7RegressionAccess::firmwareMidiOwnershipFor(p, 72) == 1,
            "fresh note lost after overlap overflow");
    send(juce::MidiMessage::noteOff(1, 72));
    const float freshTail = pump(750);
    const auto released = VDX7RegressionAccess::firmwareOwnership(p);
    require(freshTail < 1e-5f && released.midi == 0 && released.held == 0 && released.sustained == 0,
            "fresh note stuck after overlap overflow");
    require(!e.isHostResetInProgress(), "overflow test unexpectedly reset");
    unchanged(before, capture(p));
    std::cout << "PASS: retired-neighbor overflow at stage=" << stage << ", sustain=" << sustain << '\n';
}

static void testMonoRetirementFallback(const juce::File& rom)
{
    auto owner = std::make_unique<VDX7AudioProcessor>(false);
    auto& p = *owner;
    initialise(p, rom, 48000, 64);
    auto& e = VDX7RegressionAccess::engine(p);
    require(e.setPlaySetting(0, 1), "MONO fixture could not change mode");
    juce::AudioBuffer<float> audio(2, 64);
    juce::MidiBuffer midi;
    for (bool on : {true, false})
    {
        const auto message = on ? juce::MidiMessage::noteOn(1, 60, juce::uint8(100))
                                : juce::MidiMessage::noteOff(1, 60);
        e.handleMidi(message.getRawData(), message.getRawDataSize());
        for (int i = 0; i < 750; ++i) processChecked(p, audio, midi);
    }
    require(e.getPlaySetting(0) == 1 && VDX7RegressionAccess::releaseBudget(p, 60) == 1,
            "unvalidated MONO path retired its conservative history");
    std::cout << "PASS: MONO keeps conservative release history\n";
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
        const bool reactivationOnly = argc == 3 && juce::String(argv[2]) == "--reactivation-only";
        const bool historyPairOnly = argc == 3 && juce::String(argv[2]) == "--history-pair-only";
        const bool ownershipOnly = argc == 3 && juce::String(argv[2]) == "--ownership-only";
        const bool retirementOnly = argc == 3 && juce::String(argv[2]) == "--retirement-only";
        const bool overlapOnly = argc == 3 && juce::String(argv[2]) == "--overlap-only";
        const bool gateOverflowOnly = argc == 3 && juce::String(argv[2]) == "--gate-overflow-only";
        const bool expandedLifecycleOnly = argc == 3 && juce::String(argv[2]) == "--expanded-lifecycle-only";
        const bool monoNoteZeroOnly = argc == 3 && juce::String(argv[2]) == "--mono-note-zero-only";
        const bool monoBoundaryOnly = argc == 3 && juce::String(argv[2]) == "--mono-boundary-only";
        const bool profileCheckOnly = argc == 3 && juce::String(argv[2]) == "--profile-check-only";
        if ((argc != 2 && !reactivationOnly && !historyPairOnly && !ownershipOnly && !retirementOnly && !overlapOnly && !gateOverflowOnly && !expandedLifecycleOnly && !monoNoteZeroOnly && !monoBoundaryOnly && !profileCheckOnly) || !juce::File(argv[1]).existsAsFile())
            throw std::runtime_error("Supply an explicit compatible local ROM path");
        juce::MemoryBlock image;
        require(juce::File(argv[1]).loadFileAsData(image), "read explicit local ROM fixture");
        require(VDX7RegressionAccess::knownFirmwareImage(image),
                "These ownership tests require the validated v1.8 firmware (FNV1a64 20dd25e47a496ba0); other images are not validated by this suite");
        if (profileCheckOnly)
        {
            juce::MemoryBlock firmware(image.getData(), VDX7Engine::kFirmwareSize);
            require(VDX7RegressionAccess::knownFirmwareImage(firmware), "firmware-only profile");
            auto changed = firmware;
            static_cast<uint8_t*>(changed.getData())[0] ^= 1;
            require(!VDX7RegressionAccess::knownFirmwareImage(changed), "changed-image rejection");
            firmware.setSize(VDX7Engine::kFirmwareSize - 1);
            require(!VDX7RegressionAccess::knownFirmwareImage(firmware), "truncated-image rejection");
            std::cout << "PASS: required v1.8 ROM profile and rejection controls; runtime assertions are image-specific\n";
            return 0;
        }
        if (monoBoundaryOnly)
        {
            for (int mode : {0, 1})
                for (int note : {0, 1, 60, 127})
                    for (int repeats : {1, 16})
                        for (bool zeroVelocityOff : {false, true})
                            characterizeMonoBoundary(juce::File(argv[1]), mode, note, repeats, zeroVelocityOff);
            return 0;
        }
        if (monoNoteZeroOnly)
        {
            // Explicit diagnostic, not a passing release-acceptance test.
            diagnoseMonoNoteZero(juce::File(argv[1]));
            return 0;
        }
        if (expandedLifecycleOnly)
        {
            for (int mode : {0, 1})
                for (int path : {0, 1, 2})
                    testExpandedLifecycle(juce::File(argv[1]), mode, path);
            return 0;
        }
        if (gateOverflowOnly)
        {
            for (int releaseRate : {1, 99})
                for (int releaseFloor : {0, 70, 99})
                    testResetGateOverflow(juce::File(argv[1]), releaseRate, releaseFloor);
            return 0;
        }
        if (overlapOnly)
        {
            for (int rate : {44100, 48000, 96000})
                for (int block : {64, 256})
                {
                    const auto fresh = testOverlappingHistory(juce::File(argv[1]), 0, rate, block);
                    const auto history = testOverlappingHistory(juce::File(argv[1]), 16, rate, block);
                    require(history[0] <= fresh[0] + 2 && history[1] <= fresh[1] + 2,
                            "overlapping history adds more than two blocks of reset/onset delay");
                }
            for (unsigned stage : {1u, 2u, 4u})
                for (bool sustain : {false, true})
                    testRetiredHistoryOverflow(juce::File(argv[1]), stage, sustain);
            return 0;
        }
        if (retirementOnly)
        {
            testMonoRetirementFallback(juce::File(argv[1]));
            for (int rate : {44100, 48000, 96000})
                for (int block : {64, 256})
                {
                    std::array<int, 2> fresh{}, history{};
                    const auto a = testExpandedHistory(juce::File(argv[1]), 0, rate, block, true, false, &fresh);
                    const auto b = testExpandedHistory(juce::File(argv[1]), 16, rate, block, true, false, &history);
                    unchanged(a, b);
                    require(history[0] <= fresh[0] + 2 && history[1] <= fresh[1] + 2,
                            "completed history still adds more than two blocks of reset/onset delay");
                }
            return 0;
        }
        if (ownershipOnly)
        {
            for (int note : {0, 60, 127})
                for (int repeats : {1, 16})
                    for (bool sustain : {false, true})
                        testFirmwareOwnership(juce::File(argv[1]), note, repeats, sustain);
            for (unsigned stage : {1u, 2u, 4u})
                for (bool sustain : {false, true})
                    testResetAtReleaseStage(juce::File(argv[1]), stage, sustain);
            return 0;
        }
        if (historyPairOnly)
        {
            const auto fresh = testExpandedHistory(juce::File(argv[1]), 0, 48000, 64, true);
            const auto history = testExpandedHistory(juce::File(argv[1]), 16, 48000, 64, true);
            unchanged(fresh, history);
            std::cout << "PASS: matched persistent settings, idle input and adapter ownership; latency acceptance remains open\n";
            return 0;
        }
        if (reactivationOnly)
        {
            for (bool releaseFirst : {false, true})
                for (bool observed : {false, true})
                    testReactivation(juce::File(argv[1]), observed, releaseFirst);
            return 0;
        }
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
        testReactivation(juce::File(argv[1]), false);
        testReactivation(juce::File(argv[1]), true);
        for (int repeats : {1, 16})
            for (bool compact : {false, true})
                testRunningStatusRelease(juce::File(argv[1]), compact, repeats);
        testExpandedHistory(juce::File(argv[1]), 1);
        for (int rate : {44100, 48000, 96000})
            for (int block : {64, 128, 256, 512})
                testExpandedHistory(juce::File(argv[1]), 16, rate, block);
    }
    catch (const std::exception& e)
    {
        VDX7AllocationProbe::enabled = false;
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
