#include "VDX7Engine.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

// Diagnostic, not a claim that firmware responds sample-instantaneously.
// Keep the same MIDI timeline while varying host block partitioning.
static std::vector<float> render(const std::vector<uint8_t>& rom, int block, int position)
{
    VDX7Engine engine;
    if (!engine.loadRomImage(rom.data(), rom.size())) return {};
    engine.prepare(48000);
    std::vector<float> output(30000), right(30000);
    int cursor = 0;
    while (cursor < position)
    {
        const int count = std::min(block, position - cursor);
        engine.render(output.data() + cursor, right.data() + cursor, count);
        cursor += count;
    }
    const uint8_t note[] {0x90, 60, 100};
    engine.handleMidi(note, 3);
    while (cursor < static_cast<int>(output.size()))
    {
        const int count = std::min(block, static_cast<int>(output.size()) - cursor);
        engine.render(output.data() + cursor, right.data() + cursor, count);
        cursor += count;
    }
    return output;
}
int main(int argc, char** argv)
{
    if (argc != 2) return 77;
    std::ifstream file(argv[1], std::ios::binary);
    std::vector<uint8_t> rom((std::istreambuf_iterator<char>(file)), {});
    for (int position : {12000, 12016, 12064, 12128, 12256, 12384, 12511})
    {
        const auto reference = render(rom, 64, position);
        if (reference.empty()) return 77;
        int onset = -1;
        for (int i = position; i < static_cast<int>(reference.size()); ++i)
            if (std::abs(reference[i]) > 0.0001f) { onset = i; break; }
        if (onset < 0) return 1;
        std::cout << "event=" << position << " onsetDelaySamples=" << onset-position
                  << " onsetDelayMs=" << (onset-position)/48.0 << '\n';
        for (int block : {128, 256, 512})
        {
            const auto actual = render(rom, block, position);
            if (reference != actual)
            { std::cerr << "FAIL: host partition changes audio\n"; return 1; }
        }
    }
    std::cout << "PASS: identical event timelines give identical audio at 64/128/256/512 host blocks\n";
}
