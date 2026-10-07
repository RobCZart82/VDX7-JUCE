#pragma once
#include <cstddef>
#include <cstdint>

namespace VDX7FirmwareValidation
{
// Structural cold-boot mapping check only, not functional firmware validation.
inline bool resetVectorInRom(const uint8_t* firmware, std::size_t size) noexcept
{
    if (firmware == nullptr || size != 16384) return false;
    const unsigned address = (unsigned(firmware[size - 2]) << 8)
        | unsigned(firmware[size - 1]);
    return address >= 0xc000;
}
}
