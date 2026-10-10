#pragma once

#include <algorithm>

// Engine-owner-only native renderer adapter. One EGS sample is a complete
// 6-operator x 16-voice scan (96 clocks). Never switch OPS halfway through it.
// Already generated native/SRC samples keep their original mode and gain.
// No heap, queue, firmware step, mutex or host notification lives here.
class VDX7NativeSoundMode
{
public:
    static constexpr unsigned rampSamples = 256;
    static constexpr int clocksPerSample = 96;
    struct Snapshot
    {
        bool desiredClean, activeClean, quiescentInstall;
        unsigned level;
        int phase;
        float frameGain;
        bool operator==(const Snapshot&) const = default;
    };

    void request(bool clean) noexcept { desiredClean_ = clean; }
    // Only after the owner has discarded native/SRC history while inaudible.
    // Finish (but do not publish) any old partial scan; the first NEW complete
    // scan uses the latest desired mode at unity. No extra clocks or reboot.
    void installWhileQuiescent(bool clean) noexcept
    {
        desiredClean_ = clean;
        quiescentInstall_ = true;
        level_ = rampSamples;
        frameGain_ = 1.0f;
    }
    Snapshot snapshot() const noexcept
    { return { desiredClean_, activeClean_, quiescentInstall_, level_, phase_, frameGain_ }; }

    // EGS phase persists through the core's CPU boot and the wrapper's SRC
    // reset. Do not reset this adapter independently of the EGS itself.
    template<class Egs>
    void clock(Egs& egs, float* out, int& count, int cycles) noexcept
    {
        if (cycles <= 0) return;
        // Preserve the exact previous call shape for steady Classic/Clean.
        if (!quiescentInstall_ && desiredClean_ == activeClean_ && level_ == rampSamples && frameGain_ == 1.0f)
        {
            egs.clock(out, count, cycles);
            phase_ = (phase_ + cycles % clocksPerSample) % clocksPerSample;
            return;
        }
        while (cycles > 0)
        {
            if (phase_ == 0)
            {
                if (quiescentInstall_)
                {
                    activeClean_ = desiredClean_;
                    egs.clean(activeClean_);
                    quiescentInstall_ = false;
                }
                else if (desiredClean_ != activeClean_)
                {
                    if (level_ > 0) --level_;
                    if (level_ == 0)
                    {
                        activeClean_ = desiredClean_;
                        egs.clean(activeClean_); // Before the first OPS clock.
                    }
                }
                else if (level_ < rampSamples) ++level_;
                frameGain_ = static_cast<float>(level_) / static_cast<float>(rampSamples);
            }
            const int chunk = std::min(cycles, clocksPerSample - phase_);
            const int before = count;
            egs.clock(out, count, chunk);
            // A chunk never spans more than one complete scan. Gain belongs
            // to that generated sample, not to its later buffer consumption.
            if (count > before)
            {
                if (quiescentInstall_) count = before; // Old interrupted scan, not new project audio.
                else out[before] *= frameGain_;
            }
            phase_ = (phase_ + chunk) % clocksPerSample;
            cycles -= chunk; // Retain all instruction-boundary overshoot.
        }
    }

private:
    bool desiredClean_ = false, activeClean_ = false;
    bool quiescentInstall_ = false;
    unsigned level_ = rampSamples;
    int phase_ = 0;
    float frameGain_ = 1.0f;
};
