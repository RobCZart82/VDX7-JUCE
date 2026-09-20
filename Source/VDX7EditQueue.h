#pragma once
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

// Bounded MPSC sequence queue. Multiple host/UI producers, one consumer under
// engineMutex_. A reserved but unpublished slot stops consumption: commands
// cannot overtake an earlier edit. No allocation or engine lock on producers.
class VDX7EditQueue
{
public:
    enum class Kind { program, bank, op, voice };
    struct Command { Kind kind {}; int index = 0; int value = 0; };
    static constexpr std::size_t capacity = 4096;
    VDX7EditQueue() noexcept
    {
        for (std::size_t i = 0; i < capacity; ++i) slots_[i].sequence.store(i);
    }
    bool push(Command command) noexcept
    {
        // Fail closed after saturation: never accept an edit after a rejected
        // program/bank switch, which could otherwise target the wrong voice.
        auto position = write_.load(std::memory_order_relaxed);
        for (;;)
        {
            if ((position & stopped) != 0) return false;
            auto& slot = slots_[position % capacity];
            const auto sequence = slot.sequence.load(std::memory_order_acquire);
            const auto difference = static_cast<int64_t>(sequence - position);
            if (difference == 0)
            {
                if (write_.compare_exchange_weak(position, position + 1, std::memory_order_relaxed))
                {
                    slot.command = command;
                    if (command.kind == Kind::op || command.kind == Kind::voice)
                        edits_.fetch_add(1, std::memory_order_relaxed);
                    slot.sequence.store(position + 1, std::memory_order_release);
                    return true;
                }
            }
            else if (difference < 0)
            {
                // Freeze the very same reservation counter atomically. A
                // competing producer either reserved BEFORE this boundary or
                // observes stopped; no command can be accepted after the gap.
                if (write_.compare_exchange_weak(position, position | stopped,
                                                std::memory_order_acq_rel))
                    return false;
            }
            else position = write_.load(std::memory_order_relaxed);
        }
    }
    bool pop(Command& command) noexcept
    {
        const auto position = read_.load(std::memory_order_relaxed);
        auto& slot = slots_[position % capacity];
        if (slot.sequence.load(std::memory_order_acquire) != position + 1) return false;
        command = slot.command;
        if (command.kind == Kind::op || command.kind == Kind::voice)
            edits_.fetch_sub(1, std::memory_order_relaxed);
        slot.sequence.store(position + capacity, std::memory_order_release);
        read_.store(position + 1, std::memory_order_release);
        return true;
    }
    bool pending() const noexcept { return read_.load() != (write_.load() & positionMask); }
    bool hasEdits() const noexcept { return edits_.load(std::memory_order_acquire) != 0; }
    bool overflowed() const noexcept { return (write_.load(std::memory_order_acquire) & stopped) != 0; }
    // Consumer-only; called at explicit ROM/state/import transaction boundaries.
    void discard() noexcept
    {
        Command command;
        for (std::size_t i = 0; i < capacity && pop(command); ++i) {}
        auto position = write_.load();
        if (read_.load() == (position & positionMask))
            write_.compare_exchange_strong(position, position & positionMask);
    }
private:
    static_assert(sizeof(std::size_t) == 8 && std::atomic<std::size_t>::is_always_lock_free);
    static constexpr std::size_t stopped = std::size_t{1} << 63;
    static constexpr std::size_t positionMask = stopped - 1;
    struct Slot { std::atomic<std::size_t> sequence {}; Command command; };
    std::array<Slot, capacity> slots_ {};
    std::atomic<std::size_t> write_ {0}, read_ {0};
    std::atomic<std::size_t> edits_ {0};
};
