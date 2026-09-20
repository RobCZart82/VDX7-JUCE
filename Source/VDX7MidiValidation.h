#pragma once
#include <cstddef>
#include <cstdint>

namespace VDX7MidiValidation
{
// Host events contain complete messages, not a running-status byte stream.
// System common/realtime messages have no supported host action. SysEx banks
// use their own complete-message validator and never enter the serial fallback.
inline bool isChannelMessage(const uint8_t* data, std::size_t size) noexcept
{
    if (data == nullptr || size == 0 || data[0] < 0x80 || data[0] >= 0xf0)
        return false;
    const auto status = data[0] & 0xf0;
    const std::size_t expected = status == 0xc0 || status == 0xd0 ? 2 : 3;
    if (size != expected) return false;
    for (std::size_t i = 1; i < size; ++i)
        if (data[i] >= 0x80) return false;
    return true;
}
}
