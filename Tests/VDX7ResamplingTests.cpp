#include "VDX7Engine.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <numbers>
#include <stdexcept>
#include <vector>

// Inject a synthetic native-rate signal into the actual production resampler.
// No ROM, firmware execution or copyrighted preset is needed. This is an SRC
// baseline measurement, not a hardware-DX7 spectral comparison.
struct VDX7RegressionAccess
{
    static std::vector<float> sine(int rate, double frequency)
    {
        VDX7Engine e;
        e.loaded_ = true;
        e.prepare(rate);
        e.dx7_.midiVolume = 7;
        e.dx7_.midiFilter.set_f(float(10.6 / VDX7Engine::kNativeSampleRate));
        const int warmup = rate;
        std::vector<float> result(static_cast<std::size_t>(rate));
        uint64_t nativeIndex = 0;
        for (int i = 0; i < warmup + rate; ++i)
        {
            // Keep enough native samples available that generateNative is
            // never reached. Retain unread samples across refill boundaries.
            const int remaining = e.nativeCount_ - e.nativePos_;
            if (remaining < 8)
            {
                for (int j = 0; j < remaining; ++j)
                    e.nativeBlock_[j] = e.nativeBlock_[e.nativePos_ + j];
                for (int j = remaining; j < int(e.nativeBlock_.size()); ++j)
                    e.nativeBlock_[j] = float(std::sin(2 * std::numbers::pi * frequency
                        * double(nativeIndex++) / VDX7Engine::kNativeSampleRate));
                e.nativePos_ = 0; e.nativeCount_ = int(e.nativeBlock_.size());
            }
            float sample = 0;
            e.render(&sample, nullptr, 1);
            if (!std::isfinite(sample)) throw std::runtime_error("non-finite SRC output");
            if (i >= warmup) result[static_cast<std::size_t>(i - warmup)] = sample;
        }
        return result;
    }
};

static double amplitude(const std::vector<float>& samples, int rate, double frequency)
{
    double real = 0, imaginary = 0;
    for (std::size_t i = 0; i < samples.size(); ++i)
    {
        const auto phase = 2 * std::numbers::pi * frequency * double(i) / rate;
        real += samples[i] * std::cos(phase);
        imaginary -= samples[i] * std::sin(phase);
    }
    return 2 * std::hypot(real, imaginary) / double(samples.size());
}

static void checkImpulse(int rate)
{
    VDX7Resampler filter;
    filter.prepare(rate);
    int native = 0;
    double sum = 0, moment = 0;
    for (int i = 0; i < int(1000.0 * rate / VDX7Resampler::nativeRate) + filter.latency(); ++i)
    {
        const float sample = filter.sample([&] { return native++ == 512 ? 1.0f : 0.0f; });
        sum += sample; moment += i * sample;
    }
    const double expected = 512.0 * rate / VDX7Resampler::nativeRate + filter.latency();
    if (sum < 0.1 || std::abs(moment / sum - expected) > 0.02)
        throw std::runtime_error("reported latency disagrees with impulse centroid");
    filter.reset();
    for (int i = 0; i < 1000; ++i)
        if (filter.sample([] { return 0.0f; }) != 0) throw std::runtime_error("SRC reset retains audio");
    std::cout << "rate=" << rate << " latencySamples=" << filter.latency()
              << " impulseErrorSamples=" << moment / sum - expected << '\n';
}

int main()
{
    try
    {
        for (int rate : {44100, 48000, 96000})
        {
            checkImpulse(rate);
            const auto reference = VDX7RegressionAccess::sine(rate, 1000);
            const double referenceAmplitude = amplitude(reference, rate, 1000);
            if (referenceAmplitude < 0.5) throw std::runtime_error("invalid synthetic source gain");
            for (int frequency : {1000, 10000, 20000, 22200, 22500, 23000, 24000, 24500})
            {
                const auto samples = VDX7RegressionAccess::sine(rate, frequency);
                const double measuredFrequency = frequency > rate / 2.0 ? rate - frequency : frequency;
                const double gain = amplitude(samples, rate, measuredFrequency) / referenceAmplitude;
                std::cout << "rate=" << rate << " inputHz=" << frequency
                          << " measuredHz=" << measuredFrequency
                          << " relativeDb=" << 20 * std::log10(std::max(gain, 1.0e-12))
                          << (frequency > rate / 2.0 ? " ALIAS" : frequency <= 20000 ? " passband" : " transition/stopband") << '\n';
                if (!std::isfinite(gain) || gain > 1.01)
                    throw std::runtime_error("unexpected SRC gain");
                if (frequency <= 20000 && std::abs(20 * std::log10(gain)) > 0.15)
                    throw std::runtime_error("SRC passband exceeds 0.15 dB tolerance");
                if (frequency > rate / 2.0 && gain > std::pow(10.0, -70.0 / 20))
                    throw std::runtime_error("SRC folded tone exceeds -70 dB limit");
                if (rate == 96000 && frequency == 10000)
                {
                    const auto image = amplitude(samples, rate, 39096) / referenceAmplitude;
                    if (image > std::pow(10.0, -80.0 / 20))
                        throw std::runtime_error("SRC image exceeds -80 dB limit");
                    std::cout << "rate=96000 imageHz=39096 relativeDb="
                              << 20 * std::log10(std::max(image, 1.0e-12)) << '\n';
                }
            }
        }
        std::cout << "PASS: production SRC finite output, selected passband/alias/image limits, impulse latency and reset\n";
    }
    catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
