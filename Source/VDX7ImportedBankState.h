#pragma once
#include "VDX7ImportedBanks.h"

namespace VDX7ImportedBanks
{
inline constexpr int maxFileNameBytes = 1024;
struct Snapshot
{
    std::vector<Bank> banks;
    juce::String selectedId; // Empty means no imported-bank origin; program/RAM are separate.
};
// Non-realtime codec used by processor project-state capture and restore.
// No paths are opened and no sound/selection is applied. Labels are recomputed.
bool validSnapshot(const Snapshot&);
// Invalid input leaves the caller's tree untouched. Caller owns exclusive access.
bool writeState(juce::ValueTree&, const Snapshot&);
// Absent child supports legacy projects and yields an empty, absent snapshot.
// Malformed/unsupported children leave BOTH outputs untouched. Validate before
// publishing. This bounds decoded bank data, not the outer XML parser's input.
bool readState(const juce::ValueTree&, Snapshot&, bool& present);
}
