#pragma once
#include <atomic>
#include <cstdint>

// One packed display frame, read without the engine lock. UI edits merge fields;
// firmware publication is serialized by the caller's engineMutex_. Bit 63 is
// an edit-since-publication-start latch, NOT a wrapping revision counter.
class VDX7LatestDisplay
{
public:
    explicit VDX7LatestDisplay(uint64_t initial = 0) noexcept : bits_(initial) {}
    uint64_t read() const noexcept { return bits_.load(std::memory_order_acquire) & payload; }
    uint64_t update(uint64_t mask, uint64_t value) noexcept
    {
        mask &= payload;
        auto current = bits_.load(std::memory_order_acquire);
        // Even a same-value edit must invalidate an in-flight publication.
        while (!bits_.compare_exchange_weak(current, (current & ~mask) | (value & mask) | edited,
                                            std::memory_order_acq_rel, std::memory_order_acquire)) {}
        return current & payload;
    }
    template<class Capture>
    void publish(Capture capture) noexcept { publishWithHook(capture, [] {}); }
private:
    friend struct VDX7RegressionAccess;
    friend struct VDX7LatestDisplayTestAccess;
    // Deterministic test seam; no stored callback or runtime test flag.
    template<class Capture, class Hook>
    void publishWithHook(Capture capture, Hook beforeCommit) noexcept
    {
        // Single publisher only. Clear the latch BEFORE capturing engine and
        // pending UI state. Acquire also observes requests preceding UI update.
        auto expected = bits_.fetch_and(payload, std::memory_order_acq_rel) & payload;
        const auto value = capture() & payload;
        beforeCommit();
        // Any intervening UI edit sets the latch, including same-value/ABA
        // edits. Leave its coherent frame intact. Never retry on the audio
        // thread; a later normal publication refreshes other engine fields.
        (void)bits_.compare_exchange_strong(expected, value,
                                           std::memory_order_acq_rel, std::memory_order_acquire);
    }
    static_assert(std::atomic<uint64_t>::is_always_lock_free);
    static constexpr uint64_t edited = uint64_t {1} << 63;
    static constexpr uint64_t payload = edited - 1;
    std::atomic<uint64_t> bits_;
};
