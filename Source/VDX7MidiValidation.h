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

// Adapter semantics, separate from MIDI syntax. Only statically inert messages
// are excluded; unknown CCs and supported bank values retain their old route.
inline bool isIgnoredAdapterEvent(const uint8_t* data, std::size_t size) noexcept
{
    return isChannelMessage(data, size) && (data[0] & 0xf0) == 0xb0
        && (data[1] == 0 || data[1] == 100 || data[1] == 101
            || (data[1] == 32 && data[2] >= 8));
}

// Live input supports only a complete 32-voice bulk-bank message. File import
// deliberately has broader support (including single voices), but accepting an
// arbitrary F0 packet here would let invalid traffic exhaust the bounded
// deferred-MIDI storage before the engine gets a chance to reject it.
inline bool isLiveBankSysex(const uint8_t* data, std::size_t size) noexcept
{
    if (data == nullptr || size != 4104)
        return false;
    // Byte 2 is the Yamaha device/channel number (0..15), rather than the
    // host MIDI-channel filter used for ordinary channel messages.
    if (data[0] != 0xf0 || data[1] != 0x43 || (data[2] & 0xf0) != 0
        || data[3] != 0x09 || data[4] != 0x20 || data[5] != 0
        || data[4103] != 0xf7)
        return false;

    int checksum = 0;
    for (std::size_t i = 6; i <= 4102; ++i)
    {
        if (data[i] >= 0x80)
            return false;
        checksum += data[i];
    }
    return (checksum & 0x7f) == 0;
}

// Channel selection is a host-input filter, not a change to firmware routing.
// Validated bulk SysEx remains a global import, not a channel-filtered event.
inline bool acceptsHostEvent(const uint8_t* data, std::size_t size, int channel) noexcept
{
    if (data == nullptr || size == 0) return false;
    if (data[0] == 0xf0) return isLiveBankSysex(data, size);
    return isChannelMessage(data, size)
        && !isIgnoredAdapterEvent(data, size)
        && (channel == 0 || (data[0] & 15) + 1 == channel);
}
}
