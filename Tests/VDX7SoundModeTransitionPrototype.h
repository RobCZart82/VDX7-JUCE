#pragma once

// D5 experiment ONLY: audio-owner-local state; no GUI/atomic mailbox or
// serialization contract. Gain is applied to the native EGS output, before SRC.
// A mute dip is intentional. This is not a click-free or listening acceptance.
class VDX7SoundModeTransitionPrototype
{
public:
    static constexpr unsigned rampSamples = 256;
    struct Step
    {
        float gain;
        bool clean;
        bool switched;
    };

    void request(bool clean) noexcept { requestedClean_ = clean; }

    Step next() noexcept
    {
        bool switched = false;
        if (requestedClean_ != activeClean_)
        {
            if (level_ > 0)
                --level_;
            if (level_ == 0)
            {
                activeClean_ = requestedClean_;
                switched = true;
            }
        }
        else if (level_ < rampSamples)
            ++level_;
        return { static_cast<float>(level_) / static_cast<float>(rampSamples),
                 activeClean_, switched };
    }

private:
    unsigned level_ = rampSamples;
    bool requestedClean_ = false;
    bool activeClean_ = false;
};
