#pragma once
#include <juce_core/juce_core.h>
#include <array>
#include <cstdint>

// Non-realtime storage only. No firmware, PERFORMANCE globals or DAW state.
// Callers own the library directory; this class performs no UI or engine work.
class VDX7UserBank
{
public:
    using Voice = std::array<uint8_t, 128>;
    struct Snapshot
    {
        Snapshot();
        std::array<Voice, 32> voices {};
        uint32_t occupiedMask = 0;
        bool exists = false;
        bool occupied(int slot) const noexcept;
        juce::String name(int slot) const;
    };

    // Missing file is an empty bank. Corrupt/unreadable files fail without
    // replacing the caller's snapshot. Never repairs/overwrites a corrupt bank.
    static juce::Result load(const juce::File&, Snapshot& destination);

    // expected is the snapshot shown when the user chose a slot. A changed
    // target slot requires a fresh choice/confirmation. Edits to OTHER slots
    // are merged by re-reading under a process + interprocess lock.
    static juce::Result savePatch(const juce::File&, const Snapshot& expected,
                                  int slot, Voice patch, const juce::String& name,
                                  bool overwriteConfirmed);

    static constexpr const char* lockName = "VDX7-JUCE-UserBankStore-v1";
};
