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
#include "VDX7MonoTrace.h"

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
    static void checkCorrectionProfile(VDX7AudioProcessor& p)
    {
        auto& e = p.engine_;
        require(e.verifyMonoCorrectionProfile(), "six correction sites verified");
        for (const auto pc : {0xd593, 0xd645, 0xd6a8, 0xd6bb, 0xd6ce, 0xd6e0})
        {
            // Verification-only mutation, restored before any CPU execution.
            const auto saved = e.dx7_.memory[pc + 1];
            e.dx7_.memory[pc + 1] ^= 1;
            const bool accepted = e.verifyMonoCorrectionProfile();
            e.dx7_.memory[pc + 1] = saved;
            require(!accepted, "changed branch target rejected");
        }
    }
    static int monoActiveCount(const VDX7AudioProcessor& p)
    { return p.engine_.dx7_.memory[0x8e]; }
    static int monoTargetPitch(const VDX7AudioProcessor& p)
    { return VDX7MonoTrace::word(p.engine_.dx7_, 0x20d0); }
    static int monoFirstEntry(const VDX7AudioProcessor& p)
    { return VDX7MonoTrace::word(p.engine_.dx7_, 0x20b0); }
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
    static void setWheelFixture(VDX7AudioProcessor& p)
    {
        p.pitchWheelParameter_->store(1.0f);
        p.modWheelParameter_->store(0.75f);
    }
    static void applyWheels(VDX7AudioProcessor& p) { p.applyPerformanceControls(); }
    static void recoverMidi(VDX7AudioProcessor& p) { p.engine_.recoverMidiOverflow(); }
    static std::array<int, 2> wheelInputs(const VDX7AudioProcessor& p)
    { return {p.engine_.dx7_.memory[0x232a] >> 1, p.engine_.dx7_.memory[0x2337] >> 1}; }
    static void announceRestore(VDX7AudioProcessor& p)
    { p.midiTimelineEpoch_.fetch_add(1, std::memory_order_release); }
    static void installRestore(VDX7AudioProcessor& p, const juce::ValueTree& state)
    { std::scoped_lock lock(p.engineMutex_); p.restoreSavedStateLocked(state); }
    static void checkCollectedBoundary(VDX7AudioProcessor& p, const juce::ValueTree& state)
    {
        (void) p.observeStateInstall();
        juce::MidiBuffer collected;
        collected.addEvent(juce::MidiMessage::noteOn(1, 62, juce::uint8(100)), 0);
        std::size_t keyboardCount = 1;
        p.discardStaleCollectedInput(collected, keyboardCount);
        require(!collected.isEmpty() && keyboardCount == 1, "unchanged timeline dropped input");
        installRestore(p, state);
        {
            std::scoped_lock lock(p.engineMutex_);
            p.discardStaleCollectedInput(collected, keyboardCount);
        }
        require(collected.isEmpty() && keyboardCount == 0,
                "post-lock observation retained pre-install collected input");
    }
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

static double correctedProcessorPitch(const juce::File& rom, bool corrected, int key,
                                      bool mono, double sampleRate, int blockSize)
{
    auto owner = std::make_unique<VDX7AudioProcessor>(false);
    auto& p = *owner;
    auto& e = VDX7RegressionAccess::engine(p);
    require(!e.isMonoCorrectionActive(), "correction inactive before image validation");
    require(e.configureMonoCorrectionBeforeLoad(corrected), "pre-load correction selection");
    initialise(p, rom, sampleRate, blockSize);
    require(e.isMonoCorrectionActive() == corrected, "verified correction activation");
    VDX7RegressionAccess::checkCorrectionProfile(p);
    require(!e.configureMonoCorrectionBeforeLoad(!corrected)
            && e.isMonoCorrectionActive() == corrected, "reject live correction switching");
    e.setMasterTune(0);
    e.setVoiceParameter(VDX7VoiceData::VoiceParameter::transpose, 12);
    e.setOperatorParameter(0, VDX7VoiceData::Parameter::rate4, 99);
    e.reloadCurrentProgram();
    p.synchroniseOperatorParametersFromEngine();
    require(e.setPlaySetting(0, corrected && mono ? 1 : 0), "corrected MONO/native POLY fixture");
    juce::AudioBuffer<float> audio(2, blockSize);
    juce::MidiBuffer midi;
    // Preserve elapsed firmware/audio time across host block partitions.
    auto pump = [&](int baselineBlocks) {
        const int blocks = static_cast<int>(std::ceil(baselineBlocks * 64.0 / 48000.0
                                                     * sampleRate / blockSize));
        float peak = 0;
        for (int i = 0; i < blocks; ++i)
        {
            processChecked(p, audio, midi);
            if (i > blocks / 2) peak = std::max(peak, audio.getMagnitude(0, blockSize));
        }
        return peak;
    };
    auto note = [&](int n, bool on) {
        midi.addEvent(on ? juce::MidiMessage::noteOn(1, n, juce::uint8(100))
                         : juce::MidiMessage::noteOff(1, n), 0);
    };
    auto empty = [&] {
        const auto fw = VDX7RegressionAccess::firmwareOwnership(p);
        require(fw.midi == 0 && fw.held == 0 && fw.sustained == 0
                && VDX7RegressionAccess::monoActiveCount(p) == 0, "integrated ownership clears");
    };
    pump(375);
    if (corrected && mono)
    {
        // No reset or mode recovery between repeated zero notes and new input.
        for (int repeat = 0; repeat < 32; ++repeat)
        {
            note(0, true); pump(75);
            require(VDX7RegressionAccess::monoActiveCount(p) == 1, "one actual zero allocation");
            note(0, false); pump(75); empty();
        }
        note(0, true); pump(375);
        const auto zeroTarget = VDX7RegressionAccess::monoTargetPitch(p);
        note(60, true); pump(375); note(60, false); pump(375);
        require(VDX7RegressionAccess::monoTargetPitch(p) == zeroTarget
                && VDX7RegressionAccess::monoActiveCount(p) == 1, "integrated legato returns to zero");
        note(0, false); require(pump(375) < 1e-5f, "legato final silence"); empty();
    }
    note(key, true);
    double first = 0, last = 0;
    float previous = 0, peak = 0;
    int crossings = 0;
    const int measureBlocks = static_cast<int>(std::ceil(2 * sampleRate / blockSize));
    const int startBlock = static_cast<int>(std::ceil(sampleRate / blockSize));
    for (int block = 0; block < measureBlocks; ++block)
    {
        processChecked(p, audio, midi);
        if (block < startBlock) continue;
        for (int i = 0; i < blockSize; ++i)
        {
            const float value = audio.getSample(0, i);
            peak = std::max(peak, std::abs(value));
            if (previous <= 0 && value > 0 && (block != startBlock || i != 0))
            {
                const double crossing = block * blockSize + i - 1 + (-previous / (value - previous));
                if (crossings++ == 0) first = crossing;
                last = crossing;
            }
            previous = value;
        }
    }
    require(peak > 1e-4f && crossings > 2, "processor measurable audio pitch");
    const double hz = (crossings - 1) * sampleRate / (last - first);
    require(VDX7RegressionAccess::firmwareMidiOwnershipFor(p, key) == 1, "original input key owned");
    note(key, false); require(pump(375) < 1e-5f, "measured note releases to silence");
    empty();
    VDX7RegressionAccess::checkFirmwareProfile(p);
    std::cout << "PROCESSOR corrected=" << corrected << " mono=" << (corrected && mono)
              << " key=" << key << " rate=" << sampleRate << " block=" << blockSize
              << " transpose=12 hz=" << hz << '\n';
    return hz;
}

static void testCorrectedLifecycle(const juce::File& rom)
{
    // Independent fixtures: one transition must not repair another's failure.
    for (int transition = 0; transition < 5; ++transition)
    for (int history = 0; history < 3; ++history)
    {
        auto owner = std::make_unique<VDX7AudioProcessor>(false);
        auto& p = *owner;
        auto& e = VDX7RegressionAccess::engine(p);
        require(e.configureMonoCorrectionBeforeLoad(true), "lifecycle correction opt-in");
        initialise(p, rom, 48000, 64);
        e.setOperatorParameter(0, VDX7VoiceData::Parameter::rate4, 99);
        e.reloadCurrentProgram();
        require(e.setPlaySetting(0, 1), "lifecycle MONO selection");
        p.synchroniseOperatorParametersFromEngine();
        juce::AudioBuffer<float> audio(2, 64);
        juce::MidiBuffer midi;
        auto pump = [&] {
            for (int i = 0; i < 750; ++i) processChecked(p, audio, midi);
        };
        auto empty = [&] {
            const auto fw = VDX7RegressionAccess::firmwareOwnership(p);
            require(fw.midi == 0 && fw.held == 0 && fw.sustained == 0
                    && VDX7RegressionAccess::monoActiveCount(p) == 0,
                    "corrected lifecycle retained ownership");
            require(audio.getMagnitude(0, 64) < 1e-5f, "corrected lifecycle retained sound");
        };
        pump();
        juce::MemoryBlock saved;
        p.getStateInformation(saved);
        const auto before = capture(p);
        const int repetitions = history == 1 ? 16 : 1;
        if (history == 2)
            midi.addEvent(juce::MidiMessage::controllerEvent(1, 64, 127), 0);
        for (int i = 0; i < repetitions; ++i)
        {
            midi.addEvent(juce::MidiMessage::noteOn(1, 0, juce::uint8(100)), 0);
            pump();
        }
        require(VDX7RegressionAccess::monoActiveCount(p) == repetitions
                && VDX7RegressionAccess::firmwareMidiOwnershipFor(p, 0) == repetitions,
                "lifecycle requires genuinely held zero");
        if (history == 2)
        {
            // Exercise the alternate Note Off encoding while the pedal keeps
            // the voice audible; adapter emptiness alone is not acceptance.
            midi.addEvent(juce::MidiMessage::noteOn(1, 0, juce::uint8(0)), 0);
            pump();
            const auto sustained = VDX7RegressionAccess::firmwareOwnership(p);
            std::cout << "SUSTAIN transition=" << transition << " MIDI/held/sustained="
                      << sustained.midi << '/' << sustained.held << '/' << sustained.sustained
                      << " mono=" << VDX7RegressionAccess::monoActiveCount(p)
                      << " peak=" << audio.getMagnitude(0, 64) << '\n';
            require(sustained.midi == 0 && sustained.held == 0
                    && VDX7RegressionAccess::monoActiveCount(p) == 0
                    && audio.getMagnitude(0, 64) > 1e-4f,
                    "lifecycle requires genuine sustained sound");
            // MONO pedal hold is audible even with empty key slots. Prove
            // pedal release stops it, then recreate the sustained precondition.
            midi.addEvent(juce::MidiMessage::controllerEvent(1, 64, 0), 0);
            pump(); empty();
            midi.addEvent(juce::MidiMessage::controllerEvent(1, 64, 127), 0);
            midi.addEvent(juce::MidiMessage::noteOn(1, 0, juce::uint8(100)), 0);
            pump();
            midi.addEvent(juce::MidiMessage::noteOn(1, 0, juce::uint8(0)), 0);
            pump();
            require(audio.getMagnitude(0, 64) > 1e-4f, "sustain precondition recreated");
        }
        if (transition == 4)
        {
            // Audit A2: saving while sounding is not equivalent to restoring
            // the idle snapshot captured above while a key happens to be held.
            p.getStateInformation(saved);
            for (int i = 0; i < repetitions; ++i)
                midi.addEvent(juce::MidiMessage::noteOff(1, 0), 0);
            midi.addEvent(juce::MidiMessage::controllerEvent(1, 64, 0), 0);
            pump();
            empty();
            p.setStateInformation(saved.getData(), static_cast<int>(saved.getSize()));
            pump();
            const auto restored = VDX7RegressionAccess::firmwareOwnership(p);
            std::cout << "HELD-SNAPSHOT history=" << history << " MIDI/held/MONO="
                      << restored.midi << '/' << restored.held << '/'
                      << VDX7RegressionAccess::monoActiveCount(p) << std::endl;
        }
        if (transition == 0) p.reset();
        if (transition == 1) { p.releaseResources(); p.prepareToPlay(48000, 64); }
        if (transition == 2) p.setStateInformation(saved.getData(), static_cast<int>(saved.getSize()));
        if (transition == 3)
        {
            require(p.setMonoCorrectionFromUi(false), "UI disable while held");
            pump();
            require(!p.getMonoCorrectionStatus().requested && !p.getMonoCorrectionStatus().active,
                    "UI native selection status");
            empty();
            unchanged(before, capture(p));
            require(p.setMonoCorrectionFromUi(true), "UI re-enable");
        }
        pump();
        require(e.isMonoCorrectionActive(), "lifecycle lost verified correction");
        empty();
        unchanged(before, capture(p));
        for (int key : {0, 72})
        {
            midi.addEvent(juce::MidiMessage::noteOn(1, key, juce::uint8(100)), 0);
            pump();
            require(VDX7RegressionAccess::monoActiveCount(p) == 1
                    && VDX7RegressionAccess::firmwareMidiOwnershipFor(p, key) == 1
                    && audio.getMagnitude(0, 64) > 1e-4f,
                    "fresh corrected lifecycle note missing");
            midi.addEvent(juce::MidiMessage::noteOff(1, key), 0);
            pump(); empty();
        }
        VDX7RegressionAccess::checkFirmwareProfile(p);
        std::cout << "PASS: corrected held-zero lifecycle transition=" << transition
                  << " history=" << history << '\n';
    }
}

static void testCorrectionPersistence(const juce::File& rom)
{
    auto source = std::make_unique<VDX7AudioProcessor>(false);
    auto& original = VDX7RegressionAccess::engine(*source);
    require(original.configureMonoCorrectionBeforeLoad(true), "persist opt-in");
    initialise(*source, rom, 48000, 64);
    original.setOperatorParameter(0, VDX7VoiceData::Parameter::rate4, 99);
    original.reloadCurrentProgram();
    require(original.setPlaySetting(0, 1), "persist MONO");
    source->synchroniseOperatorParametersFromEngine();
    juce::AudioBuffer<float> sourceAudio(2, 64);
    juce::MidiBuffer sourceMidi;
    sourceMidi.addEvent(juce::MidiMessage::noteOn(1, 0, juce::uint8(100)), 0);
    for (int i = 0; i < 750; ++i) processChecked(*source, sourceAudio, sourceMidi);
    require(VDX7RegressionAccess::monoActiveCount(*source) == 1,
            "persistence snapshot must contain held zero");
    juce::MemoryBlock saved;
    source->getStateInformation(saved);
    auto xml = juce::AudioProcessor::getXmlFromBinary(saved.getData(), int(saved.getSize()));
    auto tree = juce::ValueTree::fromXml(*xml);
    require(bool(tree.getProperty("monoNoteZeroCorrection", false)), "saved correction intent");
    for (bool missingRom : {false, true})
    {
        auto state = tree.createCopy();
        if (missingRom) state.setProperty("romPath", "", nullptr);
        juce::MemoryBlock bytes;
        juce::AudioProcessor::copyXmlToBinary(*state.createXml(), bytes);
        auto target = std::make_unique<VDX7AudioProcessor>(false);
        auto& p = *target;
        auto& e = VDX7RegressionAccess::engine(p);
        p.setStateInformation(bytes.getData(), int(bytes.getSize()));
        require(e.isMonoCorrectionRequested(), "new instance retained intent");
        if (missingRom)
        {
            require(!e.isLoaded() && !e.isMonoCorrectionActive(), "missing ROM cannot activate");
            juce::MemoryBlock again;
            p.getStateInformation(again);
            auto held = juce::AudioProcessor::getXmlFromBinary(again.getData(), int(again.getSize()));
            require(held->getBoolAttribute("monoNoteZeroCorrection"), "missing ROM resave intent");
            require(p.loadRomFromFile(rom), "deferred correction load");
        }
        p.prepareToPlay(48000, 64);
        require(e.isMonoCorrectionActive(), "restored correction activation");
        juce::AudioBuffer<float> audio(2, 64);
        juce::MidiBuffer midi;
        auto pump = [&] { for (int i = 0; i < 750; ++i) processChecked(p, audio, midi); };
        pump();
        require(VDX7RegressionAccess::monoActiveCount(p) == 0
                && VDX7RegressionAccess::firmwareOwnership(p).midi == 0
                && VDX7RegressionAccess::firmwareOwnership(p).held == 0
                && audio.getMagnitude(0, 64) < 1e-5f,
                "fresh/deferred ROM restore must not revive saved held zero");
        midi.addEvent(juce::MidiMessage::noteOn(1, 0, juce::uint8(100)), 0); pump();
        require(VDX7RegressionAccess::monoActiveCount(p) == 1
                && audio.getMagnitude(0, 64) > 1e-4f, "restored zero sounds");
        midi.addEvent(juce::MidiMessage::noteOff(1, 0), 0); pump();
        require(VDX7RegressionAccess::monoActiveCount(p) == 0
                && VDX7RegressionAccess::firmwareOwnership(p).held == 0
                && audio.getMagnitude(0, 64) < 1e-5f, "restored zero releases");
        // A legacy project must revert a corrected instance, even with a held
        // zero and no accessible saved path. The current image is revalidated.
        midi.addEvent(juce::MidiMessage::noteOn(1, 0, juce::uint8(100)), 0); pump();
        auto legacy = tree.createCopy();
        legacy.removeProperty("monoNoteZeroCorrection", nullptr);
        legacy.setProperty("romPath", "", nullptr);
        juce::AudioProcessor::copyXmlToBinary(*legacy.createXml(), bytes);
        p.setStateInformation(bytes.getData(), int(bytes.getSize())); pump();
        require(!e.isMonoCorrectionRequested() && !e.isMonoCorrectionActive()
                && VDX7RegressionAccess::monoActiveCount(p) == 0,
                "legacy restore must clear held correction and select native");
        p.setStateInformation(saved.getData(), int(saved.getSize())); pump();
        require(e.isMonoCorrectionActive(), "restore corrected mode again");
        auto malformed = tree.createCopy();
        malformed.setProperty("monoNoteZeroCorrection", "invalid", nullptr);
        juce::AudioProcessor::copyXmlToBinary(*malformed.createXml(), bytes);
        p.setStateInformation(bytes.getData(), int(bytes.getSize()));
        require(e.isMonoCorrectionRequested() && e.isMonoCorrectionActive(), "reject malformed policy");
    }
    std::cout << "PASS: correction persistence, deferred ROM, legacy fallback and malformed policy\n";
}

static void testCorrectedCapacityAndOrder(const juce::File& rom)
{
    auto owner = std::make_unique<VDX7AudioProcessor>(false);
    auto& p = *owner;
    auto& e = VDX7RegressionAccess::engine(p);
    require(p.setMonoCorrectionFromUi(true), "capacity public correction selection");
    initialise(p, rom, 48000, 64);
    e.setOperatorParameter(0, VDX7VoiceData::Parameter::rate4, 99);
    e.reloadCurrentProgram();
    require(e.setPlaySetting(0, 1), "capacity MONO");
    juce::AudioBuffer<float> audio(2, 64);
    juce::MidiBuffer midi;
    auto pump = [&] { for (int i = 0; i < 150; ++i) processChecked(p, audio, midi); };
    auto note = [&](int key, bool on, bool zeroVelocity = false) {
        midi.addEvent(on ? juce::MidiMessage::noteOn(1, key, juce::uint8(100))
                        : zeroVelocity ? juce::MidiMessage::noteOn(1, key, juce::uint8(0))
                                       : juce::MidiMessage::noteOff(1, key), 0);
        pump();
    };
    auto empty = [&] {
        const auto fw = VDX7RegressionAccess::firmwareOwnership(p);
        require(fw.midi == 0 && fw.held == 0 && fw.sustained == 0
                && VDX7RegressionAccess::monoActiveCount(p) == 0
                && audio.getMagnitude(0, 64) < 1e-5f, "capacity/order ownership and sound cleanup");
    };
    pump();
    const auto overloads = e.midiOverloadCount();
    for (bool zeroVelocity : {false, true})
        for (int count : {1, 16, 17, 32})
        {
            for (int i = 0; i < count; ++i) note(0, true);
            require(VDX7RegressionAccess::monoActiveCount(p) == std::min(count, 16)
                    && audio.getMagnitude(0, 64) > 1e-4f, "bounded actual MONO allocation");
            for (int i = 0; i < count; ++i) note(0, false, zeroVelocity);
            empty();
            note(72, true);
            require(VDX7RegressionAccess::firmwareMidiOwnershipFor(p, 72) == 1
                    && audio.getMagnitude(0, 64) > 1e-4f, "fresh 72 after capacity history");
            note(72, false, zeroVelocity); empty();
        }
    // Establish single-note targets, then check only the unambiguous last-held
    // note. Do not assume a last-note priority for intermediate MONO states.
    const std::array<int, 3> keys {0, 60, 72};
    std::array<int, 3> targets {};
    for (int i = 0; i < 3; ++i)
    {
        note(keys[i], true); targets[i] = VDX7RegressionAccess::monoTargetPitch(p);
        note(keys[i], false); empty();
    }
    std::array<int, 3> order {0, 1, 2};
    do
    {
        std::array<int, 3> releases {0, 1, 2};
        do
        {
            for (int i : order) note(keys[i], true);
            require(VDX7RegressionAccess::monoActiveCount(p) == 3, "three real MONO allocations");
            note(keys[releases[0]], false);
            note(keys[releases[1]], false, true);
            require(VDX7RegressionAccess::monoActiveCount(p) == 1
                    && VDX7RegressionAccess::monoTargetPitch(p) == targets[releases[2]]
                    && VDX7RegressionAccess::firmwareMidiOwnershipFor(p, keys[releases[2]]) == 1
                    && audio.getMagnitude(0, 64) > 1e-4f, "last-held legato target and sound");
            note(keys[releases[2]], false); empty();
        } while (std::next_permutation(releases.begin(), releases.end()));
    } while (std::next_permutation(order.begin(), order.end()));
    require(e.midiOverloadCount() == overloads, "capacity test must not pass via overflow recovery");
    std::cout << "PASS: corrected processor 8 capacity and 36 order histories without reset\n";
}

static void testCorrectionNonzeroDifferential(const juce::File& rom)
{
    for (bool sustain : {false, true})
    for (bool porta : {false, true})
    {
        auto native = std::make_unique<VDX7AudioProcessor>(false);
        auto corrected = std::make_unique<VDX7AudioProcessor>(false);
        require(corrected->setMonoCorrectionFromUi(true), "differential opt-in");
        for (auto* p : {native.get(), corrected.get()})
        {
            initialise(*p, rom, 48000, 64);
            auto& e = VDX7RegressionAccess::engine(*p);
            e.setOperatorParameter(0, VDX7VoiceData::Parameter::rate4, 99);
            e.reloadCurrentProgram();
            require(e.setPlaySetting(0, 1), "differential MONO");
            require(e.setPlaySetting(3, porta ? 64 : 0), "differential portamento time");
        }
        juce::AudioBuffer<float> a(2, 64), b(2, 64);
        juce::MidiBuffer ma, mb;
        float maximumDifference = 0, peak = 0;
        auto pump = [&] {
            for (int block = 0; block < 150; ++block)
            {
                processChecked(*native, a, ma);
                processChecked(*corrected, b, mb);
                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < 64; ++i)
                    {
                        maximumDifference = std::max(maximumDifference,
                            std::abs(a.getSample(ch, i) - b.getSample(ch, i)));
                        peak = std::max(peak, std::abs(a.getSample(ch, i)));
                    }
                const auto x = VDX7RegressionAccess::firmwareOwnership(*native);
                const auto y = VDX7RegressionAccess::firmwareOwnership(*corrected);
                require(x.midi == y.midi && x.held == y.held && x.sustained == y.sustained
                        && VDX7RegressionAccess::monoActiveCount(*native)
                            == VDX7RegressionAccess::monoActiveCount(*corrected)
                        && VDX7RegressionAccess::monoTargetPitch(*native)
                            == VDX7RegressionAccess::monoTargetPitch(*corrected),
                        "nonzero firmware state differs between policies");
            }
        };
        auto send = [&](juce::MidiMessage msg) { ma.addEvent(msg, 0); mb.addEvent(msg, 0); pump(); };
        pump();
        send(juce::MidiMessage::controllerEvent(1, 64, sustain ? 127 : 0));
        send(juce::MidiMessage::controllerEvent(1, 65, porta ? 127 : 0));
        for (int key : {1, 60, 127, 60}) send(juce::MidiMessage::noteOn(1, key, juce::uint8(100)));
        send(juce::MidiMessage::noteOff(1, 60));
        send(juce::MidiMessage::noteOn(1, 127, juce::uint8(0)));
        send(juce::MidiMessage::noteOff(1, 1));
        send(juce::MidiMessage::noteOff(1, 60));
        send(juce::MidiMessage::controllerEvent(1, 64, 0));
        send(juce::MidiMessage::controllerEvent(1, 65, 0));
        require(peak > 1e-4f && maximumDifference <= 1e-6f,
                "nonzero native/corrected rendered audio mismatch");
        for (auto* p : {native.get(), corrected.get()})
        {
            const auto fw = VDX7RegressionAccess::firmwareOwnership(*p);
            require(fw.midi == 0 && fw.held == 0 && fw.sustained == 0
                    && VDX7RegressionAccess::monoActiveCount(*p) == 0,
                    "differential final cleanup");
        }
        require(a.getMagnitude(0, 64) < 1e-5f && b.getMagnitude(0, 64) < 1e-5f,
                "differential final silence");
        std::cout << "PASS: nonzero differential sustain=" << sustain << " porta=" << porta
                  << " maximum sample difference=" << maximumDifference << '\n';
    }
}

static void testCorrectedProcessor(const juce::File& rom)
{
    testCorrectionNonzeroDifferential(rom);
    testCorrectedCapacityAndOrder(rom);
    testCorrectionPersistence(rom);
    testCorrectedLifecycle(rom);
    for (double rate : {44100.0, 48000.0, 96000.0})
    for (int size : {64, 256})
    {
    const auto pitch = [&](bool corrected, int key, bool mono = true) {
        return correctedProcessorPitch(rom, corrected, key, mono, rate, size);
    };
    const double reference = pitch(false, 0);
    const auto pitchMatches = [reference](double actual) { return std::abs(actual / reference - 1) < 0.002; };
    require(pitchMatches(pitch(true, 0)), "integrated genuine zero pitch");
    require(!pitchMatches(pitch(true, 1)), "integrated negative pitch control must reject Note1");
    require(pitchMatches(pitch(true, 0, false)), "enabled POLY pitch control");
    const double reference72 = pitch(false, 72);
    require(std::abs(pitch(true, 72) / reference72 - 1) < 0.002,
            "subsequent 72 after zero history must match genuine reference");
    }
    std::cout << "PASS: corrected processor capacity, order, persistence, lifecycle and pitch controls (not full release acceptance)\n";
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
    VDX7MonoTrace trace;
    void verifyTraceImage() const { VDX7MonoTrace::verifyImage(machine); }
    uint16_t targetPitch() const { return VDX7MonoTrace::word(machine, 0x20d0); }
    uint16_t firstEntry() const { return VDX7MonoTrace::word(machine, 0x20b0); }
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
            const auto before = trace.enabled ? VDX7MonoTrace::state(machine) : VDX7MonoTrace::State{};
            machine.run();
            trace.observe(before, machine);
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

static void testMonoInstructionTrace(const juce::File& rom)
{
    for (int note : {0, 1})
    {
        auto raw = std::make_unique<BareFirmware>(rom);
        raw->verifyTraceImage();
        raw->setMode(1);
        raw->trace.start();
        raw->notes(note, 2, true, false);
        const auto& on = raw->trace;
        require(on.visits(0xd591) == (note == 0 ? 2 : 3), "MONO free-slot search trace");
        const auto& search = on.at(0xd591, 1);
        require(search.before.entry == ((note << 8) | 2)
                && bool(search.after.ccr & 4) == (note == 0)
                && on.at(0xd593, 1).after.pc == (note == 0 ? 0xd59b : 0xd595),
                "MONO active entry classified by zero key, not active flag");
        const auto& store = on.at(0xd59f, 1);
        require(store.before.x == (note == 0 ? 0x20b0 : 0x20b2)
                && store.before.entry == (note == 0 ? 2 : 0)
                && store.after.entry == ((note << 8) | 2), "MONO second allocation slot");
        const auto& inc = on.at(0xd5a1, 1);
        require(inc.before.count == 1 && inc.after.count == 2, "MONO allocation count transition");
        on.print(note == 0 ? "two note-0 On" : "two note-1 On control");
        raw->trace.start();
        raw->notes(note, 2, false, false);
        const auto& off = raw->trace;
        require(off.visits(0xd6b1) == 2 && off.visits(0xd645) == 2, "MONO found-key release trace");
        for (int i = 0; i < 2; ++i)
        {
            require(off.at(0xd6b1, i).after.a == note, "MONO found-key return value");
            const auto& test = off.at(0xd644, i);
            require(test.before.a == note && bool(test.after.ccr & 4) == (note == 0),
                    "MONO TSTA zero flag");
            const auto& branch = off.at(0xd645, i);
            require(branch.after.pc == (note == 0 ? 0xd666 : 0xd647), "MONO release branch destination");
        }
        require(off.visits(0xd64a) == (note == 0 ? 0 : 2)
                && off.visits(0xd651) == (note == 0 ? 0 : 2)
                && off.keyOffWrites() == (note == 0 ? 0 : 1), "MONO skipped clear/decrement/EGS Off");
        require(raw->counts() == (note == 0 ? std::array<int, 4>{0, 1, 0, 2}
                                            : std::array<int, 4>{}), "MONO trace final state");
        off.print(note == 0 ? "two note-0 Off" : "two note-1 Off control");
    }
}

struct MonoFreshReference
{
    int rawPitch, processorPitch;
    double hz;
};

static MonoFreshReference characterizeMonoContinuation(const juce::File& rom, int seed,
    int repeats, bool sequential, bool zeroVelocityOff, const MonoFreshReference* reference)
{
    auto raw = std::make_unique<BareFirmware>(rom);
    raw->verifyTraceImage();
    raw->setMode(1);
    auto owner = std::make_unique<VDX7AudioProcessor>(false);
    auto& p = *owner;
    initialise(p, rom, 48000, 64);
    auto& e = VDX7RegressionAccess::engine(p);
    e.setOperatorParameter(0, VDX7VoiceData::Parameter::rate4, 99);
    e.reloadCurrentProgram();
    p.synchroniseOperatorParametersFromEngine();
    require(e.setPlaySetting(0, 1), "MONO continuation fixture");
    const auto before = capture(p);
    juce::AudioBuffer<float> audio(2, 64);
    juce::MidiBuffer midi;
    struct Sound { float peak = 0; double hz = 0; };
    const auto send = [&](int note, int n, bool on)
    {
        raw->notes(note, n, on, zeroVelocityOff);
        for (int i = 0; i < n; ++i)
            midi.addEvent(on || zeroVelocityOff
                ? juce::MidiMessage::noteOn(1, note, juce::uint8(on ? 100 : 0))
                : juce::MidiMessage::noteOff(1, note), 0);
        Sound sound;
        int crossings = 0, frames = 0;
        float previous = 0;
        bool havePrevious = false;
        for (int block = 0; block < 375; ++block)
        {
            processChecked(p, audio, midi);
            // Observe only the settled final quarter-second of the sine fixture.
            if (block < 188) continue;
            sound.peak = std::max(sound.peak, audio.getMagnitude(0, 64));
            for (int j = 0; j < 64; ++j)
            {
                const float sample = audio.getSample(0, j);
                crossings += havePrevious && previous <= 0 && sample > 0;
                previous = sample;
                havePrevious = true;
                ++frames;
            }
        }
        sound.hz = crossings * 48000.0 / frames;
        return sound;
    };
    const auto processorCounts = [&]
    {
        const auto f = VDX7RegressionAccess::firmwareOwnership(p);
        return std::array<int, 4>{f.midi, f.held, f.sustained,
                                 VDX7RegressionAccess::monoActiveCount(p)};
    };
    require(raw->counts() == std::array<int, 4>{}
            && processorCounts() == std::array<int, 4>{}, "empty continuation fixture");
    if (sequential)
    {
        for (int i = 0; i < repeats; ++i) { send(seed, 1, true); send(seed, 1, false); }
    }
    else if (repeats != 0)
    {
        send(seed, repeats, true);
        send(seed, repeats, false);
    }
    const bool edge = seed == 0 && repeats != 0;
    const bool saturated = edge && repeats == 16;
    const auto expectedHistory = edge ? std::array<int, 4>{0, 1, 0, repeats}
                                      : std::array<int, 4>{};
    require(raw->counts() == expectedHistory && processorCounts() == expectedHistory,
            "continuation history state");
    const auto rawBeforePitch = raw->targetPitch();
    const auto processorBeforePitch = VDX7RegressionAccess::monoTargetPitch(p);

    // No reset, mode/program change or RAM edit between the bad history and
    // the next real note. A nonzero peak alone could just be the old stuck tone.
    raw->trace.start();
    const auto sound = send(72, 1, true);
    const MonoFreshReference result {raw->targetPitch(), VDX7RegressionAccess::monoTargetPitch(p), sound.hz};
    const auto& capacity = raw->trace.at(0xd58d);
    require(capacity.before.note == 72 && capacity.before.count == (edge ? repeats : 0)
            && capacity.after.pc == (saturated ? 0xd5f1 : 0xd58f), "MONO capacity branch for fresh 72");
    require(raw->trace.visits(0xd59f) == (saturated ? 0 : 1), "MONO fresh allocation trace");
    require(VDX7RegressionAccess::firmwareMidiOwnershipFor(p, 72) == 1,
            "fresh note must reach MIDI ownership even when native MONO rejects allocation");
    const auto expectedOn = std::array<int, 4>{1, 1, 0, saturated ? 16 : edge ? repeats + 1 : 1};
    require(raw->counts() == expectedOn && processorCounts() == expectedOn, "continuation fresh-on state");
    require(raw->firstEntry() == (saturated ? 2 : (72 << 8) | 2)
            && VDX7RegressionAccess::monoFirstEntry(p) == (saturated ? 2 : (72 << 8) | 2),
            "actual held voice key differs from MIDI reception record");
    if (reference != nullptr)
    {
        if (saturated)
            require(result.rawPitch == rawBeforePitch && result.processorPitch == processorBeforePitch
                    && result.rawPitch != reference->rawPitch
                    && result.processorPitch != reference->processorPitch
                    && std::abs(sound.hz - reference->hz) > 100,
                    "saturated MONO should retain old pitch, not sound fresh 72");
        else
            require(result.rawPitch == reference->rawPitch
                    && result.processorPitch == reference->processorPitch
                    && std::abs(sound.hz - reference->hz) < 8 && sound.peak > 1e-4f,
                    "accepted fresh 72 must match target pitch and audible reference");
    }
    else
        require(sound.peak > 1e-4f && sound.hz > 500 && sound.hz < 600, "audible clean note-72 sine reference");

    if (saturated && !sequential && !zeroVelocityOff) raw->trace.print("fresh 72 blocked by count 16");
    raw->trace.start();
    const auto releasedSound = send(72, 1, false);
    const auto expectedOff = !edge ? std::array<int, 4>{}
        : saturated ? std::array<int, 4>{0, 1, 0, 16} : std::array<int, 4>{0, 0, 0, 1};
    require(raw->counts() == expectedOff && processorCounts() == expectedOff,
            "continuation fresh-off state");
    require(raw->trace.keyOffWrites() == (edge ? 0 : 1), "continuation EGS Off observation");
    require(edge ? releasedSound.peak > 1e-4f : releasedSound.peak < 1e-5f,
            "continuation known stuck output versus released control");
    raw->trace.stop();
    unchanged(before, capture(p));
    std::cout << "CONTINUATION CHARACTERIZATION (not a fix): seed=" << seed << " repeats=" << repeats
              << " sequential=" << sequential << " velocity-zero-off=" << zeroVelocityOff
              << " pitch72=" << result.processorPitch << " measuredHz=" << sound.hz
              << " releasePeak=" << releasedSound.peak << " finalMidi/held/mono="
              << expectedOff[0] << '/' << expectedOff[1] << '/' << expectedOff[3] << '\n';
    return result;
}

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

static std::vector<float> renderDeferredPartition(const juce::File& rom, int rate,
                                                 const std::vector<int>& partitions, int ignoredCC = -1)
{
    auto owner = std::make_unique<VDX7AudioProcessor>(false);
    auto& p = *owner;
    initialise(p, rom, rate, 64);
    auto& e = VDX7RegressionAccess::engine(p);
    e.setOperatorParameter(0, VDX7VoiceData::Parameter::rate4, 99);
    e.reloadCurrentProgram();
    p.synchroniseOperatorParametersFromEngine();
    juce::AudioBuffer<float> blocked(2, 64);
    juce::MidiBuffer midi;
    for (int i = 0; i < 100; ++i) processChecked(p, blocked, midi);
    const auto before = capture(p);
    // Create actual processor contention, not an injected queue/counter state.
    if (ignoredCC >= 0)
        for (int i = 0; i < 300; ++i)
            midi.addEvent(juce::MidiMessage::controllerEvent(1, ignoredCC, 127), 0);
    midi.addEvent(juce::MidiMessage::noteOn(1, 72, juce::uint8(100)), 16);
    {
        std::unique_lock lock(VDX7RegressionAccess::mutex(p));
        auto callback = std::async(std::launch::async, [&] { processChecked(p, blocked, midi); });
        const bool timely = callback.wait_for(std::chrono::seconds(1)) == std::future_status::ready;
        if (!timely) { lock.unlock(); callback.get(); }
        require(timely, "partition fixture callback blocked on mutex");
        callback.get();
    }
    require(blocked.getMagnitude(0, 64) == 0 && VDX7RegressionAccess::deferred(p),
            "partition fixture did not defer the note");
    const int total = rate * 3, offTime = rate / 2;
    std::vector<float> output(static_cast<std::size_t>(total) * 2);
    int start = 0;
    std::size_t part = 0;
    while (start < total)
    {
        const int size = std::min(partitions[part++ % partitions.size()], total - start);
        juce::AudioBuffer<float> audio(2, size);
        // A future host Off must stay on the same 64-sample-delayed timeline.
        if (offTime >= start && offTime < start + size)
            midi.addEvent(juce::MidiMessage::noteOff(1, 72), offTime - start);
        processChecked(p, audio, midi);
        for (int ch = 0; ch < 2; ++ch)
            std::copy_n(audio.getReadPointer(ch), size, output.data() + ch * total + start);
        start += size;
    }
    float peak = 0, tail = 0;
    for (int i = 0; i < total; ++i)
    {
        peak = std::max(peak, std::abs(output[i]));
        if (i >= total - rate / 4) tail = std::max(tail, std::abs(output[i]));
    }
    const auto owned = VDX7RegressionAccess::firmwareOwnership(p);
    std::cout << "Deferred partition rate=" << rate << ", first block=" << partitions[0]
              << ", peak=" << peak << ", tail=" << tail << '\n';
    require(peak > 1e-4f, "large successful block lost deferred Note On");
    require(tail < 1e-5f && owned.midi == 0 && owned.held == 0 && owned.sustained == 0
            && !e.hasHeldMidiNotes() && !VDX7RegressionAccess::deferred(p),
            "partitioned deferred note failed to release");
    unchanged(before, capture(p));
    return output;
}

static void testWheelDelivery(const juce::File& rom)
{
    bool allPassed = true;
    for (int schedule : {0, 1, 2})
    {
        auto owner = std::make_unique<VDX7AudioProcessor>(false);
        auto& p = *owner;
        initialise(p, rom, 48000, 64);
        juce::AudioBuffer<float> audio(2, 64);
        juce::MidiBuffer midi;
        for (int i = 0; i < 750; ++i) processChecked(p, audio, midi);
        VDX7RegressionAccess::setWheelFixture(p);
        if (schedule == 1) VDX7RegressionAccess::recoverMidi(p);
        VDX7RegressionAccess::applyWheels(p);
        if (schedule == 2) VDX7RegressionAccess::recoverMidi(p);
        for (int i = 0; i < 3000; ++i) processChecked(p, audio, midi);
        const auto actual = VDX7RegressionAccess::wheelInputs(p);
        const bool passed = actual == std::array<int, 2>{127, 95};
        std::cout << "Wheel delivery schedule=" << schedule << " pitch/mod="
                  << actual[0] << '/' << actual[1] << " expected=127/95 "
                  << (passed ? "PASS" : "FAIL") << std::endl;
        if (schedule == 0) require(passed, "wheel delivery control fixture invalid");
        allPassed &= passed;
        midi.addEvent(juce::MidiMessage::pitchWheel(1, 32 * 128), 0);
        midi.addEvent(juce::MidiMessage::controllerEvent(1, 1, 19), 1);
        for (int i = 0; i < 750; ++i) processChecked(p, audio, midi);
        require(VDX7RegressionAccess::wheelInputs(p) == std::array<int, 2>{32, 19},
                "newer physical wheel input must supersede unchanged GUI values");
        VDX7RegressionAccess::recoverMidi(p);
        for (int i = 0; i < 3000; ++i) processChecked(p, audio, midi);
        require(VDX7RegressionAccess::wheelInputs(p) == std::array<int, 2>{32, 19},
                "recovery replayed stale GUI over newer physical wheel input");
    }
    require(allPassed, "wheel intent lost on rejection or queued-message flush");
}

static void testBypassedRelease(const juce::File& rom)
{
    for (bool pedal : {false, true})
    {
        auto owner = std::make_unique<VDX7AudioProcessor>(false);
        auto& p = *owner;
        initialise(p, rom, 48000, 64);
        auto& e = VDX7RegressionAccess::engine(p);
        e.setOperatorParameter(0, VDX7VoiceData::Parameter::rate4, 99);
        e.reloadCurrentProgram(); p.synchroniseOperatorParametersFromEngine();
        juce::AudioBuffer<float> audio(2, 64);
        juce::MidiBuffer midi;
        if (pedal) midi.addEvent(juce::MidiMessage::controllerEvent(1, 64, 127), 0);
        midi.addEvent(juce::MidiMessage::noteOn(1, 60, juce::uint8(100)), 1);
        for (int i = 0; i < 750; ++i) processChecked(p, audio, midi);
        require(audio.getMagnitude(0, 64) > 1e-4f, "bypass fixture needs audible held note");
        midi.addEvent(juce::MidiMessage::noteOff(1, 60), 0);
        midi.addEvent(juce::MidiMessage::controllerEvent(1, 64, 0), 1);
        for (int i = 0; i < 750; ++i)
        {
            VDX7AllocationProbe::allocations = VDX7AllocationProbe::deallocations = 0;
            VDX7AllocationProbe::enabled = true;
            p.processBlockBypassed(audio, midi);
            VDX7AllocationProbe::enabled = false;
            require(VDX7AllocationProbe::allocations == 0 && VDX7AllocationProbe::deallocations == 0,
                    "bypass callback allocates/deallocates");
            require(audio.getMagnitude(0, 64) == 0, "bypass must be silent");
            require(midi.isEmpty(), "synth bypass must consume MIDI");
            // Hosts deliver each block once; do not replay unconsumed input.
            midi.clear();
        }
        for (int i = 0; i < 750; ++i) processChecked(p, audio, midi);
        const auto fw = VDX7RegressionAccess::firmwareOwnership(p);
        require(!e.hasHeldMidiNotes() && fw.midi == 0 && fw.held == 0
                && audio.getMagnitude(0, 64) < 1e-5f,
                "bypass lost NoteOff/pedal release and revived held note");
        midi.addEvent(juce::MidiMessage::noteOn(1, 72, juce::uint8(100)), 0);
        for (int i = 0; i < 750; ++i) processChecked(p, audio, midi);
        require(audio.getMagnitude(0, 64) > 1e-4f, "post-bypass note must sound");
        midi.addEvent(juce::MidiMessage::noteOff(1, 72), 0);
        for (int i = 0; i < 750; ++i) processChecked(p, audio, midi);
        require(audio.getMagnitude(0, 64) < 1e-5f && !e.hasHeldMidiNotes(), "post-bypass release");
    }
    std::cout << "PASS: bypass note/pedal release and fresh playback\n";
}

static void testDeferredPartitions(const juce::File& rom)
{
    testBypassedRelease(rom);
    {
        auto owner = std::make_unique<VDX7AudioProcessor>(false);
        auto& p = *owner;
        initialise(p, rom, 48000, 64);
        juce::MemoryBlock saved;
        p.getStateInformation(saved);
        const auto xml = juce::AudioProcessor::getXmlFromBinary(saved.getData(), int(saved.getSize()));
        const auto state = juce::ValueTree::fromXml(*xml);
        VDX7RegressionAccess::checkCollectedBoundary(p, state);
        juce::AudioBuffer<float> audio(2, 64);
        juce::MidiBuffer midi;
        // Deterministic schedule: announcement observed, input deferred, then
        // actual state installed. Uses real callback and restore implementation.
        VDX7RegressionAccess::announceRestore(p);
        {
            std::unique_lock lock(VDX7RegressionAccess::mutex(p));
            midi.addEvent(juce::MidiMessage::programChange(1, 7), 0);
            midi.addEvent(juce::MidiMessage::noteOn(1, 62, juce::uint8(100)), 2);
            auto callback = std::async(std::launch::async, [&] { processChecked(p, audio, midi); });
            const bool timely = callback.wait_for(std::chrono::seconds(1)) == std::future_status::ready;
            if (!timely) lock.unlock();
            callback.get();
            require(timely, "restore schedule blocked callback");
        }
        require(VDX7RegressionAccess::deferred(p), "restore schedule needs deferred input");
        VDX7RegressionAccess::installRestore(p, state);
        for (int i = 0; i < 750; ++i) processChecked(p, audio, midi);
        require(VDX7RegressionAccess::engine(p).currentProgram() == int(state.getProperty("program"))
                && !VDX7RegressionAccess::engine(p).hasHeldMidiNotes(),
                "pre-install MIDI replayed into restored project");
        midi.addEvent(juce::MidiMessage::noteOn(1, 72, juce::uint8(100)), 0);
        for (int i = 0; i < 750; ++i) processChecked(p, audio, midi);
        require(audio.getMagnitude(0, 64) > 1e-4f, "fresh post-install note lost");
        midi.addEvent(juce::MidiMessage::noteOff(1, 72), 0);
        for (int i = 0; i < 750; ++i) processChecked(p, audio, midi);
        require(!VDX7RegressionAccess::engine(p).hasHeldMidiNotes(), "fresh post-install release lost");
        std::cout << "PASS: announcement/deferred-input/install boundary\n";
    }
    for (int cc : {0, 100, 101, 32})
    {
        std::cout << "Ignored CC contention fixture: " << cc << std::endl;
        renderDeferredPartition(rom, 48000, {64}, cc);
    }
    for (int rate : {44100, 48000, 96000})
    {
        const auto reference = renderDeferredPartition(rom, rate, {64});
        for (const auto& partitions : {std::vector<int>{1024}, std::vector<int>{rate * 2},
                                      std::vector<int>{rate * 3}, std::vector<int>{17, 63, 4096, 1, 255}})
        {
            const auto output = renderDeferredPartition(rom, rate, partitions);
            float difference = 0;
            for (std::size_t i = 0; i < reference.size(); ++i)
                difference = std::max(difference, std::abs(reference[i] - output[i]));
            require(difference < 1e-6f, "deferred audio depends on successful host-block partition");
        }
    }
    std::cout << "PASS: deferred note onset/release and stereo samples invariant across successful block partitions\n";
}

static void testDeferredExpiry(const juce::File& rom, int resetPath, int skippedSamples)
{
    auto owner = std::make_unique<VDX7AudioProcessor>(false);
    auto& p = *owner;
    initialise(p, rom, 48000, 64);
    auto& e = VDX7RegressionAccess::engine(p);
    e.setOperatorParameter(0, VDX7VoiceData::Parameter::rate4, 99);
    e.reloadCurrentProgram();
    p.synchroniseOperatorParametersFromEngine();
    juce::AudioBuffer<float> small(2, 64), skipped(2, skippedSamples), zero(2, 0);
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::controllerEvent(1, 64, 127), 0);
    midi.addEvent(juce::MidiMessage::noteOn(1, 60, juce::uint8(100)), 1);
    for (int i = 0; i < 500; ++i) processChecked(p, small, midi);
    require(small.getMagnitude(0, 64) > 1e-4f, "expiry fixture lacks a sounding held note");
    const auto before = capture(p);
    if (resetPath != 0) resetChecked(p);
    if (resetPath == 2)
    {
        // Enter the actual reset-drain branch without completing it. Next
        // callback must not receive playback credit merely for owning the lock.
        processChecked(p, zero, midi);
        require(e.isHostResetInProgress(), "expiry fixture reset did not remain in progress");
    }
    midi.addEvent(juce::MidiMessage::noteOff(1, 60), 0);
    midi.addEvent(juce::MidiMessage::noteOn(1, 72, juce::uint8(100)), 1);
    if (resetPath == 0)
    {
        std::unique_lock lock(VDX7RegressionAccess::mutex(p));
        auto callback = std::async(std::launch::async, [&] { processChecked(p, skipped, midi); });
        const bool timely = callback.wait_for(std::chrono::seconds(1)) == std::future_status::ready;
        if (!timely) { lock.unlock(); callback.get(); }
        require(timely, "expiry callback blocked on mutex");
        callback.get();
    }
    else processChecked(p, skipped, midi);
    require(skipped.getMagnitude(0, skippedSamples) == 0, "skipped expiry block was not silent");
    processChecked(p, zero, midi);
    // Drain the panic and all firmware releases. The expired fresh note must
    // never be accepted, even though the recovery block itself is large.
    juce::AudioBuffer<float> recovered(2, 144000);
    processChecked(p, recovered, midi);
    const auto owned = VDX7RegressionAccess::firmwareOwnership(p);
    require(!e.hasHeldMidiNotes() && owned.midi == 0 && owned.held == 0 && owned.sustained == 0
            && recovered.getMagnitude(0, 132000, 12000) < 1e-5f,
            "expired deferred input replayed or lost held-note/sustain reconciliation");
    midi.addEvent(juce::MidiMessage::noteOn(1, 74, juce::uint8(100)), 0);
    for (int i = 0; i < 500; ++i) processChecked(p, small, midi);
    require(VDX7RegressionAccess::firmwareMidiOwnershipFor(p, 74) == 1
            && small.getMagnitude(0, 64) > 1e-4f, "valid note after expiry did not sound");
    midi.addEvent(juce::MidiMessage::noteOff(1, 74), 0);
    for (int i = 0; i < 1000; ++i) processChecked(p, small, midi);
    require(!e.hasHeldMidiNotes() && small.getMagnitude(0, 64) < 1e-5f,
            "fresh note after expiry did not release");
    unchanged(before, capture(p));
    std::cout << "PASS: true delay expiry reset path=" << resetPath << ", skipped=" << skippedSamples
              << "; sustain/held release and fresh input recover\n";
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
        const bool monoCorrectedOnly = argc == 3 && juce::String(argv[2]) == "--mono-corrected-processor-only";
        const bool monoBoundaryOnly = argc == 3 && juce::String(argv[2]) == "--mono-boundary-only";
        const bool monoTraceOnly = argc == 3 && juce::String(argv[2]) == "--mono-trace-only";
        const bool profileCheckOnly = argc == 3 && juce::String(argv[2]) == "--profile-check-only";
        const bool deferredPartitionOnly = argc == 3 && juce::String(argv[2]) == "--deferred-partition-only";
        const bool wheelDeliveryOnly = argc == 3 && juce::String(argv[2]) == "--wheel-delivery-only";
        if ((argc != 2 && !reactivationOnly && !historyPairOnly && !ownershipOnly && !retirementOnly && !overlapOnly && !gateOverflowOnly && !expandedLifecycleOnly && !monoNoteZeroOnly && !monoCorrectedOnly && !monoBoundaryOnly && !monoTraceOnly && !profileCheckOnly && !deferredPartitionOnly && !wheelDeliveryOnly) || !juce::File(argv[1]).existsAsFile())
            throw std::runtime_error("Supply an explicit compatible local ROM path");
        juce::MemoryBlock image;
        require(juce::File(argv[1]).loadFileAsData(image), "read explicit local ROM fixture");
        require(VDX7RegressionAccess::knownFirmwareImage(image),
                "These ownership tests require the validated v1.8 firmware (FNV1a64 20dd25e47a496ba0); other images are not validated by this suite");
        if (monoCorrectedOnly) { testCorrectedProcessor(juce::File(argv[1])); return 0; }
        if (wheelDeliveryOnly) { testWheelDelivery(juce::File(argv[1])); return 0; }
        if (deferredPartitionOnly)
        {
            testDeferredPartitions(juce::File(argv[1]));
            for (int path : {0, 1, 2})
                for (int skipped : {96001, 144000})
                    testDeferredExpiry(juce::File(argv[1]), path, skipped);
            return 0;
        }
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
        if (monoTraceOnly)
        {
            testMonoInstructionTrace(juce::File(argv[1]));
            const auto reference = characterizeMonoContinuation(juce::File(argv[1]), 1, 0, false, false, nullptr);
            for (int seed : {0, 1})
                for (int repeats : {1, 16})
                    for (bool sequential : {false, true})
                        for (bool zeroVelocityOff : {false, true})
                            characterizeMonoContinuation(juce::File(argv[1]), seed, repeats,
                                                         sequential, zeroVelocityOff, &reference);
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
