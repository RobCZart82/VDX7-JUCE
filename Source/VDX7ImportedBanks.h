#pragma once

#include "VDX7FactoryBanks.h"
#include <functional>

namespace VDX7ImportedBanks
{
inline constexpr int maxDirectoryEntries = 512;
inline constexpr int maxFiles = 128;
inline constexpr int maxDisplayCharacters = 15;
struct Bank
{
    juce::String contentId; // SHA-256 of validated 4096-byte VMEM, not its name/channel.
    juce::String fileName;  // Exact filename for future tooltip/disambiguation.
    juce::String displayName;
    std::vector<uint8_t> packed;
};
enum class ScanStatus { complete, cancelled, limitReached, pathError };
struct ScanResult
{
    std::vector<Bank> banks;
    juce::StringArray warnings;
    ScanStatus status = ScanStatus::complete;
    bool complete() const noexcept { return status == ScanStatus::complete; }
};
juce::File defaultFolder(); // Does not create a folder or scan it.
juce::String displayName(const juce::String& fileName);
// Blocking, non-recursive, non-audio-thread operation. No runtime publication
// or processor changes here. Cancellation/limit/path errors discard partial banks.
// shouldCancel must be a cheap thread-safe query, not a UI or file operation.
ScanResult scan(const juce::File&, const std::function<bool()>& shouldCancel = {},
                const VDX7FactoryBanks::ReferenceHashes& = VDX7FactoryBanks::references());
}
