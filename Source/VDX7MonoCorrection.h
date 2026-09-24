#pragma once
#include <cstdint>

// Opt-in compatibility policy; the engine remains native by default. Pure decision
// logic: no ROM/RAM writes, MIDI rewriting, allocation, CPU stepping or locks.
// The caller must verify the image/instruction map before setting profile=true.
namespace VDX7MonoCorrection
{
struct Input
{
    bool enabled = false, profile = false, mono = false, nativeZ = false;
    uint16_t pc = 0, slotAddress = 0;
    uint8_t key = 0, flags = 0, a = 0, b = 0, requestedKey = 0;
};
struct Decision { int site; bool z; };

constexpr bool validSlot(uint16_t address) noexcept
{
    return address >= 0x20b0 && address < 0x20d0 && (address & 1) == 0;
}

constexpr Decision evaluate(const Input& in) noexcept
{
    const Decision unchanged {-1, in.nativeZ};
    if (!in.enabled || !in.profile || !in.mono) return unchanged;
    const bool slot = validSlot(in.slotAddress);
    const bool occupied = slot && (in.flags & 2) != 0;
    switch (in.pc)
    {
        case 0xd593: return slot ? Decision{0, !occupied} : unchanged;
        case 0xd645: return {1, !(in.b != 0 && occupied && in.key == in.requestedKey)};
        case 0xd6a8: return {2, occupied && in.a == in.key};
        case 0xd6bb: return slot ? Decision{3, !occupied} : unchanged;
        case 0xd6ce: return slot ? Decision{4, !occupied} : unchanged;
        case 0xd6e0: return slot ? Decision{5, !occupied} : unchanged;
        default: return unchanged;
    }
}
}
