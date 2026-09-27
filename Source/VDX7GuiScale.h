#pragma once

#include <array>
#include <cstddef>

namespace VDX7GuiScale
{
struct Preset
{
    int percentage;
    int width;
    int height;
};

inline constexpr std::array<Preset, 5> presets
{{
    { 50, 600, 463 },
    { 75, 900, 694 },
    { 100, 1200, 925 },
    { 125, 1500, 1156 },
    { 150, 1800, 1388 }
}};

constexpr int indexForWidth(int width) noexcept
{
    int bestIndex = 0;
    auto bestDistance = width > presets[0].width
        ? width - presets[0].width : presets[0].width - width;
    for (int i = 1; i < static_cast<int>(presets.size()); ++i)
    {
        const auto candidate = presets[static_cast<std::size_t>(i)].width;
        const auto distance = width > candidate ? width - candidate : candidate - width;
        if (distance < bestDistance)
        {
            bestIndex = i;
            bestDistance = distance;
        }
    }
    return bestIndex;
}

constexpr const Preset& atIndex(int index) noexcept
{
    const auto safeIndex = index < 0 ? 0
        : index >= static_cast<int>(presets.size()) ? static_cast<int>(presets.size()) - 1
                                                    : index;
    return presets[static_cast<std::size_t>(safeIndex)];
}
}
