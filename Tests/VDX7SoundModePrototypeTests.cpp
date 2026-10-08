// D5 measurement harness only. No production mode switch or firmware is used.
#include "EGS.h"
#include "VDX7SoundModeTransitionPrototype.h"
#include "VDX7SoundModeStatePrototype.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace
{
void require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

struct Metrics
{
    double rms = 0.0, peak = 0.0;
};

template <typename T>
Metrics metrics(const std::vector<T>& samples)
{
    require(!samples.empty(), "measurement must not be empty");
    Metrics result;
    for (auto sample : samples)
    {
        const double value = static_cast<double>(sample);
        require(std::isfinite(value), "all samples must be finite");
        result.rms += value * value;
        result.peak = std::max(result.peak, std::abs(value));
    }
    result.rms = std::sqrt(result.rms / samples.size());
    return result;
}

template <typename T>
double differenceRms(const std::vector<T>& a, const std::vector<T>& b)
{
    require(a.size() == b.size() && !a.empty(), "comparison lengths must match");
    double sum = 0.0;
    for (std::size_t i = 0; i < a.size(); ++i)
    {
        const double delta = static_cast<double>(a[i]) - static_cast<double>(b[i]);
        sum += delta * delta;
    }
    return std::sqrt(sum / a.size());
}

// Exercise the public OPS API with own register values, not DX7 preset bytes.
struct OpsFixture
{
    std::uint16_t frequency[6][16] {};
    std::uint16_t envelope[6][16] {};
    dx7Emu::OPS ops { frequency, envelope };

    OpsFixture(int algorithm, int feedback, int pitch, int attenuation)
    {
        for (int op = 0; op < 6; ++op)
            for (int voice = 0; voice < 16; ++voice)
            {
                frequency[op][voice] = static_cast<std::uint16_t>(pitch + op * 37);
                envelope[op][voice] = static_cast<std::uint16_t>(attenuation);
            }
        ops.setAlgorithm(0x30, static_cast<std::uint8_t>((algorithm << 3) | feedback));
        ops.keyOn(0);
    }

    std::vector<std::int32_t> render(int count)
    {
        std::vector<std::int32_t> result;
        result.reserve(static_cast<std::size_t>(count));
        for (int i = 0; i < count; ++i)
        {
            for (int op = 0; op < 6; ++op)
                ops.clock(op, 0);
            result.push_back(ops.out[0]);
        }
        return result;
    }
};

void conversionCharacterization()
{
    dx7Emu::ExpTab table;
    unsigned changed = 0;
    // Exhaust the inverse-log table for both polarities. Expected quantization
    // uses division/multiplication rather than calling upstream shift().
    for (unsigned index = 0; index < 16384; ++index)
        for (bool negative : { false, true })
        {
            dx7Emu::logsin_t value(static_cast<std::uint16_t>(index));
            value.sign = negative;
            const auto magnitude = table.get14(index);
            const unsigned quantum = magnitude >= 4096 ? 8 : magnitude >= 2048 ? 4
                                                     : magnitude >= 1024 ? 2 : 1;
            const auto classicMagnitude = (magnitude / quantum) * quantum;
            const int sign = negative ? -1 : 1;
            require(table.invertLogSinClean(value) == sign * static_cast<int>(magnitude),
                    "Clean retains the pre-shift magnitude and polarity");
            require(table.invertLogSin(value) == sign * static_cast<int>(classicMagnitude),
                    "Classic applies the magnitude-dependent low-bit quantization");
            if (magnitude != classicMagnitude)
                ++changed;
        }
    require(changed > 0, "conversion test must exercise differing magnitudes");
    std::cout << "conversion: differing table entries (both polarities)=" << changed << '\n';
}

void operatorCharacterization(bool ignoreClean)
{
    int changedCases = 0, cases = 0;
    double largestDifference = 0.0;
    for (int algorithm = 0; algorithm < 32; ++algorithm)
        for (int feedback : { 0, 7 })
            for (int attenuation : { 0, 128, 1023 })
            {
                OpsFixture baseline(algorithm, feedback, 12000, attenuation);
                OpsFixture classic(algorithm, feedback, 12000, attenuation);
                OpsFixture clean(algorithm, feedback, 12000, attenuation);
                OpsFixture toggled(algorithm, feedback, 12000, attenuation);
                classic.ops.clean(false);
                clean.ops.clean(!ignoreClean);
                baseline.render(256);
                classic.render(256);
                clean.render(256);
                toggled.render(256);
                // No clock between requests: a flag alone must not reset OPS state.
                toggled.ops.clean(true);
                toggled.ops.clean(false);
                const auto original = baseline.render(2048);
                require(original == classic.render(2048), "default OPS is exactly Classic");
                require(original == toggled.render(2048), "unclocked round trip preserves OPS state");
                const auto alternate = clean.render(2048);
                metrics(original);
                metrics(alternate);
                const double difference = differenceRms(original, alternate);
                if (difference > 0.0)
                    ++changedCases;
                largestDifference = std::max(largestDifference, difference);
                ++cases;
            }
    require(changedCases > 0, "Clean must change at least one nonzero OPS stimulus");
    std::cout << "OPS: cases=" << cases << " differing=" << changedCases
              << " largest raw-unit difference RMS=" << largestDifference << '\n';
}

// Public EGS registers: six parallel carriers, fixed pitches, fast envelope.
// Construct EGS first (it initializes its register memory), then set own data.
struct EgsFixture
{
    std::array<std::uint8_t, 256> memory {};
    dx7Emu::EGS egs { memory.data() };

    EgsFixture(int voices, int pitch)
    {
        memory.fill(0);
        for (int op = 0; op < 6; ++op)
        {
            const auto fixedPitch = static_cast<unsigned>((pitch + op * 37) << 2) | 1u;
            memory[0x20 + op * 2] = static_cast<std::uint8_t>(fixedPitch >> 8);
            memory[0x21 + op * 2] = static_cast<std::uint8_t>(fixedPitch);
            egs.update(static_cast<std::uint8_t>(0x21 + op * 2));
            for (int stage = 0; stage < 4; ++stage)
            {
                memory[0x40 + op * 4 + stage] = 63;
                memory[0x60 + op * 4 + stage] = stage == 3 ? 63 : 0;
            }
            for (int voice = 0; voice < 16; ++voice)
                memory[0x80 + op * 16 + voice] = voice < voices ? 0 : 255;
            egs.update(static_cast<std::uint8_t>(0x43 + op * 4));
            egs.update(static_cast<std::uint8_t>(0x63 + op * 4));
        }
        egs.setAlgorithm(0x30, 31 << 3);
        for (int voice = 0; voice < 16; ++voice)
        {
            egs.update(static_cast<std::uint8_t>(voice * 2 + 1));
            if (voice < voices)
            {
                memory[0xF1] = static_cast<std::uint8_t>((voice << 2) | 1);
                egs.update(0xF1);
            }
        }
    }

    std::vector<float> render(int count)
    {
        std::vector<float> result(static_cast<std::size_t>(count));
        int written = 0;
        egs.clock(result.data(), written, count * 96);
        require(written == count, "EGS emits one sample per 96 clocks in both modes");
        metrics(result);
        return result;
    }
};

void outputCharacterization()
{
    for (int voices : { 1, 16 })
        for (int pitch : { 9000, 14000 })
        {
            EgsFixture baseline(voices, pitch), classic(voices, pitch), clean(voices, pitch);
            EgsFixture repeat(voices, pitch), toggled(voices, pitch);
            classic.egs.clean(false);
            clean.egs.clean(true);
            repeat.egs.clean(true);
            baseline.render(2048);
            classic.render(2048);
            clean.render(2048);
            repeat.render(2048);
            toggled.render(2048);
            toggled.egs.clean(true);
            toggled.egs.clean(false);
            const auto original = baseline.render(8192);
            const auto alternate = clean.render(8192);
            require(original == classic.render(8192), "default EGS is exactly Classic");
            require(original == toggled.render(8192), "unclocked round trip preserves EGS state");
            require(alternate == repeat.render(8192), "Clean is deterministic per independent instance");
            const auto a = metrics(original), b = metrics(alternate);
            require(a.rms > 0.0 && b.rms > 0.0, "both EGS paths must produce nonzero signal");
            require(differenceRms(original, alternate) > 0.0, "EGS paths must not be identical");
            std::cout << "EGS: voices=" << voices << " pitch-register=" << pitch
                      << " Classic RMS/peak=" << a.rms << '/' << a.peak
                      << " Clean RMS/peak=" << b.rms << '/' << b.peak
                      << " difference RMS=" << differenceRms(original, alternate) << '\n';
            // A real rendered transition, not a click-free acceptance assertion.
            EgsFixture transition(voices, pitch), peer(voices, pitch), peerControl(voices, pitch);
            transition.render(2048);
            peer.render(2048);
            peerControl.render(2048);
            const auto before = transition.render(512);
            transition.egs.clean(true);
            const auto during = transition.render(512);
            transition.egs.clean(false);
            const auto after = transition.render(512);
            const auto uninterrupted = peer.render(1536);
            require(uninterrupted == peerControl.render(1536), "mode requests do not affect a peer EGS");
            std::cout << "  abrupt boundary step Classic->Clean=" << std::abs(during.front() - before.back())
                      << " Clean->Classic=" << std::abs(after.front() - during.back())
                      << " uninterrupted Classic steps=" << std::abs(uninterrupted[512] - uninterrupted[511])
                      << '/' << std::abs(uninterrupted[1024] - uninterrupted[1023])
                      << " return first-sample delta=" << std::abs(after.front() - uninterrupted[1024]) << '\n';
        }
}

void transitionStateCharacterization()
{
    using Transition = VDX7SoundModeTransitionPrototype;
    Transition idle;
    for (int i = 0; i < 1024; ++i)
    {
        const auto step = idle.next();
        require(step.gain == 1.0f && !step.clean && !step.switched,
                "idle transition must preserve default Classic at unity gain");
    }

    Transition transition;
    for (bool wanted : { true, false })
    {
        transition.request(wanted);
        unsigned switches = 0;
        float previousGain = 1.0f;
        for (unsigned i = 0; i < 2 * Transition::rampSamples; ++i)
        {
            transition.request(wanted); // Repeated identical requests must not restart the ramp.
            const auto step = transition.next();
            require(step.gain >= 0.0f && step.gain <= 1.0f,
                    "transition gain stays bounded");
            require(std::abs(step.gain - previousGain) <= 1.0f / Transition::rampSamples,
                    "transition gain changes by at most one fixed step");
            if (step.switched)
            {
                ++switches;
                require(i == Transition::rampSamples - 1 && step.gain == 0.0f,
                        "single mode request switches exactly at the mute boundary");
            }
            if (i == 2 * Transition::rampSamples - 1)
                require(step.clean == wanted && step.gain == 1.0f,
                        "single request settles within 512 native samples");
            previousGain = step.gain;
        }
        require(switches == 1, "single request changes mode exactly once");
    }

    Transition cancellation;
    cancellation.request(true);
    for (int i = 0; i < 96; ++i)
        require(!cancellation.next().switched, "mode must not switch before the mute boundary");
    cancellation.request(false);
    for (int i = 0; i < 96; ++i)
    {
        const auto step = cancellation.next();
        require(!step.clean && !step.switched, "cancelled request must not apply a stale mode");
        if (i == 95)
            require(step.gain == 1.0f, "cancelled ramp returns to unity without a mode change");
    }

    // Latest request wins; repeated requests cannot grow a queue or reset gain.
    Transition rapid;
    float previousGain = 1.0f;
    for (int i = 0; i < 4096; ++i)
    {
        const bool wanted = (i % 31) < 17;
        rapid.request(wanted);
        const auto step = rapid.next();
        require(step.gain >= 0.0f && step.gain <= 1.0f
                    && std::abs(step.gain - previousGain) <= 1.0f / Transition::rampSamples,
                "rapid requests preserve the bounded gain trajectory");
        require(!step.switched || step.gain == 0.0f,
                "rapid requests only switch at the mute boundary");
        require(!step.switched || step.clean == wanted,
                "rapid requests apply the latest mode rather than an earlier pending value");
        previousGain = step.gain;
    }
    for (bool wanted : { true, false })
    {
        rapid.request(wanted);
        Transition::Step final {};
        for (unsigned i = 0; i < 2 * Transition::rampSamples; ++i)
            final = rapid.next();
        require(final.clean == wanted && final.gain == 1.0f,
                "latest request settles within the bound after requests stop");
    }
}

struct TransitionFrame
{
    float raw, output, gain;
    bool clean, switched;
    bool operator==(const TransitionFrame&) const = default;
};

std::vector<TransitionFrame> transitionTrace(int voices, int pitch, int partition,
                                            bool requests, bool unmutedSwitch)
{
    EgsFixture fixture(voices, pitch);
    fixture.render(2048);
    VDX7SoundModeTransitionPrototype transition;
    std::vector<TransitionFrame> result;
    constexpr int total = 2048;
    result.reserve(total); // Measurement storage allocated before the sample loop.
    for (int blockStart = 0; blockStart < total; blockStart += partition)
        for (int i = blockStart; i < std::min(total, blockStart + partition); ++i)
        {
            // Events have exact absolute native-sample offsets in this test;
            // this is not a host/GUI scheduling or atomic request implementation.
            if (requests && (i == 0 || i == 1024))
                transition.request(i == 0);
            const auto step = transition.next();
            if (step.switched)
                fixture.egs.clean(step.clean);
            float raw = 0.0f;
            int written = 0;
            fixture.egs.clock(&raw, written, 96);
            require(written == 1 && std::isfinite(raw), "transition clocks exactly one finite EGS sample");
            const float gain = unmutedSwitch && step.switched ? 1.0f : step.gain;
            const float output = raw * gain;
            require(!step.switched || (gain == 0.0f && output == 0.0f),
                    "mode changes must be rendered at zero output");
            result.push_back({ raw, output, gain, step.clean, step.switched });
        }
    return result;
}

void transitionOutputCharacterization(bool unmutedSwitch)
{
    for (int voices : { 1, 16 })
        for (int pitch : { 9000, 14000 })
        {
            EgsFixture control(voices, pitch);
            control.render(2048);
            const auto original = control.render(2048);
            const auto idle = transitionTrace(voices, pitch, 64, false, false);
            for (std::size_t i = 0; i < idle.size(); ++i)
                require(std::bit_cast<std::uint32_t>(idle[i].output) == std::bit_cast<std::uint32_t>(original[i]),
                        "idle transition wrapper is bit-identical to uninterrupted Classic EGS");
            const auto trace = transitionTrace(voices, pitch, 64, true, unmutedSwitch);
            for (int partition : { 1, 7, 511, 2048 })
                require(trace == transitionTrace(voices, pitch, partition, true, false),
                        "same native-offset requests are invariant across synthetic partitions");

            unsigned switches = 0;
            double peak = 0.0;
            for (std::size_t i = 0; i < trace.size(); ++i)
            {
                const auto& frame = trace[i];
                require(std::isfinite(frame.output) && std::abs(frame.output) <= std::abs(frame.raw),
                        "transition never amplifies its raw EGS sample");
                peak = std::max(peak, static_cast<double>(std::abs(frame.output)));
                if (frame.switched)
                {
                    ++switches;
                    require(i > 0 && frame.gain == 0.0f && frame.output == 0.0f,
                            "rendered mode boundary is muted");
                    const float previous = trace[i - 1].output;
                    require(std::abs(previous) <= std::abs(trace[i - 1].raw) / 256.0f,
                            "sample immediately before a mode change has at most one-step gain");
                    std::cout << "transition: voices=" << voices << " pitch-register=" << pitch
                              << " native-offset=" << i << " clean=" << frame.clean
                              << " raw boundary step=" << std::abs(frame.raw - trace[i - 1].raw)
                              << " ramped boundary step=" << std::abs(frame.output - previous) << '\n';
                }
            }
            require(switches == 2 && !trace.back().clean && trace.back().gain == 1.0f,
                    "rendered trace completes both requests and returns to full-gain Classic");
            std::cout << "  ramped trace peak=" << peak << " (raw upstream units, not host clipping verdict)\n";
        }
}

void stateContractCharacterization(bool replayStaleAsCurrent)
{
    namespace State = VDX7SoundModeStatePrototype;
    using Mode = State::Mode;
    State::Owner first, second;
    require(first.snapshot().desired == Mode::classic && !first.rendererRequest(),
            "new instance is Classic with no request to an absent engine");
    require(first.requestFromUi(Mode::clean, first.snapshot().projectRevision),
            "a current UI may request Clean before the first ROM");
    require(first.capture() == State::Encoded { "1", "1" } && !first.rendererRequest(),
            "no-ROM save preserves desired Clean without a renderer dispatch");
    const auto saved = first.capture();
    require(second.restore(saved.version, saved.mode, false), "model restores valid pending Clean");
    require(second.capture() == saved && !second.rendererRequest(),
            "pending save preserves desired Clean and does not touch an unrelated renderer");
    require(second.completeCompatibleRestore(second.snapshot().projectRevision), "current compatible restore completes");
    require(second.rendererRequest() == Mode::clean, "compatible restore dispatches desired Clean");
    second.loseEngine();
    require(second.capture() == saved && !second.rendererRequest(),
            "losing the model engine preserves desired mode");

    // Rejection is whole-transaction preservation, not a fallback mutation.
    for (std::string_view bad : { "", "2", "-1", "+1", "01", "1 ", " 1", "1.0", "1x", "true", "Clean" })
    {
        const auto before = second.snapshot();
        require(!second.restore("1", bad, true) && second.snapshot() == before,
                "malformed mode rejects without changing desired/revision/pending/readiness");
        require(!second.restore(bad, "1", true) && second.snapshot() == before,
                "malformed or unknown feature version rejects without mutation");
    }
    const auto beforePartial = second.snapshot();
    require(!second.restore(std::nullopt, "1", true) && second.snapshot() == beforePartial,
            "mode without its feature version rejects atomically");
    require(!second.restore("1", std::nullopt, true) && second.snapshot() == beforePartial,
            "feature version without its mode rejects atomically");

    const auto staleUi = first.snapshot();
    require(first.restore(std::nullopt, std::nullopt, true), "legacy project is accepted");
    require(first.capture() == State::Encoded { "1", "0" } && first.rendererRequest() == Mode::classic,
            "legacy restore after Clean explicitly selects Classic");
    const auto submittedRevision = replayStaleAsCurrent ? first.snapshot().projectRevision : staleUi.projectRevision;
    require(!first.requestFromUi(Mode::clean, submittedRevision),
            "a UI token from before project recall cannot overwrite the recalled mode");
    require(first.rendererRequest() == Mode::classic, "stale UI rejection preserves recalled Classic");

    const auto currentRevision = first.snapshot().projectRevision;
    require(first.requestFromUi(Mode::clean, currentRevision)
                && first.requestFromUi(Mode::classic, currentRevision)
                && first.requestFromUi(Mode::clean, currentRevision),
            "current-project UI requests coalesce to the latest mode");
    require(first.capture() == saved, "save before audio observation captures the latest desired mode");
    require(second.restore("1", "0", false), "pending Classic project is accepted");
    require(first.capture() == saved && second.capture() == State::Encoded { "1", "0" },
            "project modes remain instance-local");
    const auto oldCompletion = second.snapshot().projectRevision;
    require(second.restore("1", "1", false), "a newer pending Clean project is accepted");
    const auto beforeLateCompletion = second.snapshot();
    require(!second.completeCompatibleRestore(oldCompletion) && second.snapshot() == beforeLateCompletion,
            "an older pending completion cannot mark a newer project engine-ready");
    const auto pendingRevision = second.snapshot().projectRevision;
    require(second.requestFromUi(Mode::classic, pendingRevision)
                && second.capture() == State::Encoded { "1", "0" } && !second.rendererRequest(),
            "pending mode edit changes saved intent without dispatching to the renderer");
    require(second.requestFromUi(Mode::clean, pendingRevision), "current pending project accepts mode edit");
    require(second.capture() == saved && !second.rendererRequest(), "pending edit is saved but not dispatched");
    require(second.completeCompatibleRestore(pendingRevision), "latest compatible restore completes");
    require(second.rendererRequest() == Mode::clean, "pending edit survives compatible restore completion");

    const auto beforeInvalidUi = first.snapshot();
    require(!first.requestFromUi(static_cast<Mode>(255), currentRevision)
                && first.snapshot() == beforeInvalidUi, "invalid internal UI mode rejects without mutation");
    State::Owner exhausted(std::numeric_limits<std::uint64_t>::max());
    const auto beforeExhaustion = exhausted.snapshot();
    require(!exhausted.restore("1", "1", true) && exhausted.snapshot() == beforeExhaustion,
            "project revision exhaustion must not reuse a stale token");

    // Desired project value stays Clean while an audio-local ramp is still Classic.
    VDX7SoundModeTransitionPrototype transition;
    transition.request(first.rendererRequest() == Mode::clean);
    const auto inFlight = transition.next();
    require(!inFlight.clean && inFlight.gain < 1.0f && first.capture() == saved,
            "save captures desired mode rather than transient active mode or gain");
    std::cout << "state contract: legacy/strict decode, no-ROM/pending save, stale-UI ordering and isolation PASS\n";
}
} // namespace

int main(int argc, char** argv)
{
    const bool negativeControl = argc == 2 && std::string_view(argv[1]) == "--ignore-clean-negative-control";
    const bool unmutedSwitch = argc == 2 && std::string_view(argv[1]) == "--unmuted-switch-negative-control";
    const bool staleUi = argc == 2 && std::string_view(argv[1]) == "--stale-ui-negative-control";
    if (argc != 1 && !negativeControl && !unmutedSwitch && !staleUi)
    {
        std::cerr << "Usage: vdx7_sound_mode_prototype_tests [--ignore-clean-negative-control | --unmuted-switch-negative-control | --stale-ui-negative-control]\n";
        return 2;
    }
    try
    {
        conversionCharacterization();
        operatorCharacterization(negativeControl);
        outputCharacterization();
        transitionStateCharacterization();
        transitionOutputCharacterization(unmutedSwitch);
        stateContractCharacterization(staleUi);
        std::cout << "PASS: ROM-free upstream characterization only; no shipped switch or sound-quality verdict\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
