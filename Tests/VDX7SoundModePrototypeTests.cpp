// D5 measurement harness only. No production mode switch or firmware is used.
#include "EGS.h"

#include <algorithm>
#include <array>
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
} // namespace

int main(int argc, char** argv)
{
    const bool negativeControl = argc == 2 && std::string_view(argv[1]) == "--ignore-clean-negative-control";
    if (argc != 1 && !negativeControl)
    {
        std::cerr << "Usage: vdx7_sound_mode_prototype_tests [--ignore-clean-negative-control]\n";
        return 2;
    }
    try
    {
        conversionCharacterization();
        operatorCharacterization(negativeControl);
        outputCharacterization();
        std::cout << "PASS: ROM-free upstream characterization only; no shipped switch or sound-quality verdict\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
