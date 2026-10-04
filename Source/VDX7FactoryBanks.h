#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <array>
#include <cstdint>
#include <vector>

namespace VDX7FactoryBanks
{
inline constexpr std::size_t bankSize = 4096;
inline constexpr std::size_t imageSize = 8 * bankSize;
inline constexpr int maxFiles = 128;
using ReferenceHashes = std::array<juce::String, 8>;
struct Snapshot
{
    std::vector<uint8_t> image;
    uint8_t mask = 0;
};
struct ScanResult
{
    Snapshot banks;
    juce::StringArray warnings;
    bool complete = true; // Incomplete scans must not replace a live catalog.
};

const ReferenceHashes& references();
juce::File defaultFolder();
// Hash the complete validated VMEM payload, never its filename/header/channel.
int identify(const std::vector<uint8_t>& packed, const ReferenceHashes& hashes = references());
ScanResult scan(const juce::File& folder, const ReferenceHashes& hashes = references());
bool valid(const Snapshot&);
void writeState(juce::ValueTree&, const Snapshot&);
// Missing properties are a legacy state. A malformed pair is not a legacy state.
bool readState(const juce::ValueTree&, Snapshot&, bool& present);
}
