#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <vector>

// Causal, fractional-delay Blackman-windowed sinc. Tables are prepared only
// off the audio thread. Rendering uses fixed history and no allocation/locks.
class VDX7Resampler
{
public:
    static constexpr double nativeRate = 49096.0;
    static constexpr int taps = 321, phases = 256;
    VDX7Resampler() { prepare(48000); }
    void prepare(double rate)
    {
        rate = std::isfinite(rate) && rate > 1000 ? rate : 48000;
        if (rate != rate_ || coefficients_.empty())
        {
            rate_ = rate;
            step_ = nativeRate / rate;
            latency_ = static_cast<int>(std::ceil(128 * rate / nativeRate));
            const double delay = latency_ * step_; // Exact integer host latency.
            const double cutoff = 0.475 * std::min(1.0, rate / nativeRate);
            coefficients_.resize((phases + 1) * taps);
            for (int phase = 0; phase <= phases; ++phase)
            {
                double sum = 0;
                for (int k = 0; k < taps; ++k)
                {
                    const double d = k + double(phase) / phases - delay;
                    const double x = 2 * cutoff * d;
                    const double sinc = std::abs(x) < 1e-12 ? 1 : std::sin(std::numbers::pi * x) / (std::numbers::pi * x);
                    const double window = std::abs(d) > 126 ? 0 :
                        0.42 + 0.5 * std::cos(std::numbers::pi * d / 126)
                             + 0.08 * std::cos(2 * std::numbers::pi * d / 126);
                    const double value = 2 * cutoff * sinc * window;
                    coefficients_[phase * taps + k] = static_cast<float>(value);
                    sum += value;
                }
                for (int k = 0; k < taps; ++k)
                    coefficients_[phase * taps + k] /= static_cast<float>(sum);
            }
        }
        reset();
    }
    void reset() noexcept { history_.fill(0); head_ = 0; phase_ = 0; primed_ = false; }
    int latency() const noexcept { return latency_; }
    template<class Next> float sample(Next next)
    {
        if (!primed_) { push(next()); primed_ = true; }
        const double position = phase_ * phases;
        const int p = std::min(phases - 1, static_cast<int>(position));
        const float mix = static_cast<float>(position - p);
        const auto* a = coefficients_.data() + p * taps;
        const auto* b = a + taps;
        float output = 0;
        for (int k = 0; k < taps; ++k)
            output += history_[head_ + k] * (a[k] + mix * (b[k] - a[k]));
        phase_ += step_;
        while (phase_ >= 1)
        { push(next()); phase_ -= 1; }
        return output;
    }
private:
    void push(float value) noexcept
    {
        head_ = head_ == 0 ? taps - 1 : head_ - 1;
        history_[head_] = history_[head_ + taps] = value;
    }
    std::vector<float> coefficients_;
    std::array<float, taps * 2> history_ {};
    double rate_ = 0, step_ = 1, phase_ = 0;
    int latency_ = 0, head_ = 0;
    bool primed_ = false;
};
