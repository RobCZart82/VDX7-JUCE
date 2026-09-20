#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

// Audio-owned delayed timeline. Input advances during contention; playback
// advances only when rendering. Later MIDI joins the same shifted timeline.
// Fixed storage and bounded compaction; overflow requires note reconciliation.
class VDX7DeferredMidi
{
public:
    bool push(const uint8_t* data, std::size_t size, int samplePosition = 0) noexcept
    {
        if (panic_) return false;
        if (size == 0) return true;
        const auto time = inputTime_ + static_cast<uint64_t>(samplePosition < 0 ? 0 : samplePosition);
        if (count_ == events_.size() || size > bytes_.size() - used_
            || (count_ != 0 && time < events_[count_ - 1].time))
        {
            clear();
            panic_ = true;
            return false;
        }
        events_[count_++] = {used_, size, time};
        std::memcpy(bytes_.data() + used_, data, size);
        used_ += size;
        return true;
    }
    void advanceInputBlock(int samples, uint64_t maximumLag) noexcept
    {
        inputTime_ += static_cast<uint64_t>(samples > 0 ? samples : 0);
        if (inputTime_ - playbackTime_ > maximumLag)
        { clear(); panic_ = true; }
    }
    template<class Event, class Panic>
    void renderBlock(int samples, Event event, Panic panic)
    {
        if (panic_) { clear(); panic(); return; }
        const auto end = playbackTime_ + static_cast<uint64_t>(samples > 0 ? samples : 0);
        std::size_t consumed = 0, consumedBytes = 0;
        while (consumed < count_ && events_[consumed].time <= end)
        {
            const auto& e = events_[consumed++];
            const int position = e.time > playbackTime_ ? static_cast<int>(e.time - playbackTime_) : 0;
            event(bytes_.data() + e.offset, e.size, position);
            consumedBytes = e.offset + e.size;
        }
        playbackTime_ = end;
        if (consumed != 0)
        {
            for (std::size_t i = consumed; i < count_; ++i)
            {
                events_[i - consumed] = events_[i];
                events_[i - consumed].offset -= consumedBytes;
            }
            count_ -= consumed;
            used_ -= consumedBytes;
            std::memmove(bytes_.data(), bytes_.data() + consumedBytes, used_);
        }
    }
    // Only reset after notes and sustain are released, not between a delayed
    // note-on and a future note-off which has not arrived from the host yet.
    void resetIfEmpty() noexcept { if (count_ == 0 && !panic_) clear(); }
    bool active() const noexcept { return panic_ || count_ != 0 || inputTime_ != playbackTime_; }
    void clear() noexcept
    { count_ = used_ = 0; inputTime_ = playbackTime_ = 0; panic_ = false; }
private:
    struct Entry { std::size_t offset, size; uint64_t time; };
    std::array<Entry, 256> events_ {};
    std::array<uint8_t, 65536> bytes_ {};
    std::size_t count_ = 0, used_ = 0;
    uint64_t inputTime_ = 0, playbackTime_ = 0;
    bool panic_ = false;
};
