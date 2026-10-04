#pragma once

#include <juce_core/juce_core.h>

namespace VDX7StateBytes
{
// JUCE's decoder allocates from a decimal prefix, not from the text length.
// Validate our fixed-size, canonical state format before any allocation.
inline bool decode(const juce::String& text, std::size_t expectedSize, juce::MemoryBlock& output)
{
    const auto prefix = juce::String(static_cast<unsigned int>(expectedSize)) + ".";
    const auto encodedSize = prefix.length() + static_cast<int>((expectedSize * 8 + 5) / 6);
    if (text.length() != encodedSize || !text.startsWith(prefix)) return false;
    juce::MemoryBlock candidate;
    if (!candidate.fromBase64Encoding(text) || candidate.getSize() != expectedSize
        || candidate.toBase64Encoding() != text) return false;
    output = std::move(candidate);
    return true;
}
}
