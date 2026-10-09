#pragma once

#include <juce_data_structures/juce_data_structures.h>

// Off-audio-thread codec. PluginProcessor uses the reader for fail-closed
// admission; live Clean projects remain gated on renderer/lifecycle integration.
// Normal writer/Clean dispatch remain gated; explicit pending Classic metadata
// uses the processor's captured project owner. Callers must
// capture a coherent tree + desired mode under their own ownership protection.
namespace VDX7SoundModeState
{
enum class Mode { classic = 0, clean = 1 };
inline const juce::Identifier rootType { "VDX7STATE" };
inline const juce::Identifier versionProperty { "soundModeVersion" };
inline const juce::Identifier modeProperty { "soundMode" };

inline bool isValid(Mode mode) noexcept
{
    return mode == Mode::classic || mode == Mode::clean;
}

inline bool isExactScalar(const juce::var& value, const char* text)
{
    // XML properties become strings. Do not accept bool/double/array/object or
    // permissive numeric prefixes in an in-memory tree. XML cannot retain the
    // original var type; only the decoded representation can be validated.
    return (value.isInt() || value.isInt64() || value.isString())
        && value.toString() == text;
}

// Failure leaves the caller's desired mode unchanged. This validates ONLY the
// D5 pair/root, not RAM, ROM identity, other metadata or the whole project.
inline bool read(const juce::ValueTree& state, Mode& desired)
{
    if (!state.hasType(rootType)) return false;
    const bool hasVersion = state.hasProperty(versionProperty);
    const bool hasMode = state.hasProperty(modeProperty);
    if (!hasVersion && !hasMode)
    {
        desired = Mode::classic;
        return true;
    }
    if (!hasVersion || !hasMode || !isExactScalar(state[versionProperty], "1"))
        return false;
    const auto value = state[modeProperty];
    if (isExactScalar(value, "0")) desired = Mode::classic;
    else if (isExactScalar(value, "1")) desired = Mode::clean;
    else return false;
    return true;
}

// Returns a deep detached copy, never mutates the input or its shared handles.
// Overwrites only this pair with the captured latest desired value, including
// for a pending snapshot. No DSP dispatch, ramp advancement or runtime fields.
// Invalid root/enum returns an invalid tree and leaves the input untouched.
inline juce::ValueTree writeDetached(const juce::ValueTree& captured, Mode desired)
{
    if (!captured.hasType(rootType) || !isValid(desired)) return {};
    auto encoded = captured.createCopy();
    encoded.setProperty(versionProperty, 1, nullptr);
    encoded.setProperty(modeProperty, static_cast<int>(desired), nullptr);
    return encoded;
}
} // namespace VDX7SoundModeState
