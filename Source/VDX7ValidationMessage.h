#pragma once
#include "VDX7VoiceData.h"
#include <string>

// Non-RT presentation. No input bytes or personal file paths are printed.
inline std::string vdx7ValidationMessage(const VDX7VoiceData::ValidationResult& result)
{
    if (result.ok()) return {};
    if (result.code == VDX7VoiceData::ValidationCode::invalidInput)
        return "Invalid packed voice input: expected complete 128-byte voices.";
    return "Bank " + std::to_string(result.voice / 32 + 1)
        + ", voice " + std::to_string(result.voice % 32 + 1)
        + ", byte " + std::to_string(result.byte)
        + " (zero-based), " + result.field + ": value " + std::to_string(result.value)
        + "; allowed " + std::to_string(result.minimum) + ".." + std::to_string(result.maximum) + ".";
}
