#pragma once
#include <array>
#include <atomic>
#include <cstdint>

// Producer calls are serialized by MidiKeyboardState's own lock. Audio is
// the only consumer. Never acquire that UI lock from the audio callback.
class VDX7KeyboardQueue
{
public:
    using Event = std::array<uint8_t, 3>;
    static constexpr uint32_t capacity = 256;
    bool push(Event event) noexcept
    {
        if (overflow_.load(std::memory_order_acquire)) return false;
        const auto write = write_.load(std::memory_order_relaxed);
        if (write - read_.load(std::memory_order_acquire) == capacity)
        { overflow_.store(true, std::memory_order_release); return false; }
        events_[write % capacity] = event;
        write_.store(write + 1, std::memory_order_release);
        return true;
    }
    bool pop(Event& event) noexcept
    {
        const auto read = read_.load(std::memory_order_relaxed);
        if (read == write_.load(std::memory_order_acquire)) return false;
        event = events_[read % capacity];
        read_.store(read + 1, std::memory_order_release);
        return true;
    }
    bool recoverOverflow() noexcept
    {
        if (!overflow_.load(std::memory_order_acquire)) return false;
        // The producer stops publishing while overflow is set.
        read_.store(write_.load(std::memory_order_acquire), std::memory_order_release);
        overflow_.store(false, std::memory_order_release);
        return true;
    }
    void discard() noexcept
    {
        Event ignored;
        for (uint32_t i = 0; i < capacity && pop(ignored); ++i) {}
        recoverOverflow();
    }
private:
    static_assert(std::atomic<uint32_t>::is_always_lock_free);
    std::array<Event, capacity> events_ {};
    std::atomic<uint32_t> write_ {0}, read_ {0};
    std::atomic<bool> overflow_ {false};
};
