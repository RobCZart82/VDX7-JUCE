#pragma once
#include <cstdint>
#include <vector>

namespace VDX7Sysex
{
// Decode one complete VCED (163 bytes) or VMEM (4104 bytes) message.
// Output is one packed voice or a packed bank; failure leaves output unchanged.
bool decode(const std::vector<uint8_t>& message, std::vector<uint8_t>& packed);
std::vector<uint8_t> encode(const std::vector<uint8_t>& packed);
}
