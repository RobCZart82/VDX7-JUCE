#pragma once

#include <juce_core/juce_core.h>

namespace VDX7StatusPresentation
{
inline juce::String choose(const juce::String& critical, bool hasUnexportedEdits,
                           const juce::String& ordinary)
{
    if (critical.isNotEmpty()) return critical;
    if (hasUnexportedEdits) return "Bank has unexported edits\nSAVE AS... to export";
    return ordinary;
}
}
