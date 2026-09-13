#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

// Audio-thread-owned, bounded storage for events received during an engine
// transaction. Overflow discards the batch and requires note reconciliation;
// it must never replay a note-on whose corresponding note-off was dropped.
class VDX7DeferredMidi
{
public:
    bool push(const uint8_t* data, std::size_t size) noexcept
    {
        if (panic_) return false;
        if (size == 0) return true;
        if (count_ == events_.size() || size > bytes_.size() - used_)
        {
            clear();
            panic_ = true;
            return false;
        }
        events_[count_++] = {used_, size};
        std::memcpy(bytes_.data() + used_, data, size);
        used_ += size;
        return true;
    }
    template<class Event, class Panic>
    void drain(Event event, Panic panic)
    {
        if (panic_) panic();
        else for (std::size_t i = 0; i < count_; ++i)
            event(bytes_.data() + events_[i].offset, events_[i].size);
        clear();
    }
    void clear() noexcept { count_ = used_ = 0; panic_ = false; }
private:
    struct Entry { std::size_t offset, size; };
    std::array<Entry, 256> events_ {};
    std::array<uint8_t, 65536> bytes_ {};
    std::size_t count_ = 0, used_ = 0;
    bool panic_ = false;
};
