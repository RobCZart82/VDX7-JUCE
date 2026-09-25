#pragma once

#include <juce_core/juce_core.h>
#include <cstdint>
#include <limits>
#include <vector>

namespace VDX7BoundedFile
{
// Read only a known-format file after checking its length, and never allocate
// more than maxBytes. The extra-byte probe also rejects a file that grew while
// it was being read. On failure, destination is empty.
inline bool read(const juce::File& file, std::size_t maxBytes,
                 std::vector<uint8_t>& destination)
{
    destination.clear();
    if (!file.existsAsFile())
        return false;

    juce::FileInputStream stream(file);
    if (!stream.openedOk())
        return false;

    const auto length = stream.getTotalLength();
    if (length < 0 || static_cast<uint64_t>(length) > maxBytes
        || static_cast<uint64_t>(length) > static_cast<uint64_t>(std::numeric_limits<int>::max()))
        return false;

    destination.resize(static_cast<std::size_t>(length));
    std::size_t offset = 0;
    while (offset < destination.size())
    {
        const auto remaining = static_cast<int>(destination.size() - offset);
        const int count = stream.read(destination.data() + offset, remaining);
        if (count <= 0)
        {
            destination.clear();
            return false;
        }
        offset += static_cast<std::size_t>(count);
    }

    char extra = 0;
    if (stream.read(&extra, 1) != 0)
    {
        destination.clear();
        return false;
    }
    return true;
}
}
