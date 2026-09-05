#pragma once
#include <array>
#include <algorithm>
#include <cstdint>

// Operator indices are OP1=0 ... OP6=5. inputs[target] is a source bitmask.
// Derived by symbolic signal tracing of Retromulator dx7Lib OPS::algoROM,
// preserving multi-operator feedback (algorithms 4 and 6). See THIRD_PARTY.md.
namespace VDX7Algorithms
{
struct Graph
{
    std::array<uint8_t, 6> inputs;
    uint8_t carriers;
    int feedbackFrom, feedbackTo;
};
inline constexpr std::array<Graph, 32> graphs {{
    {{2, 0, 8, 16, 32, 0}, 5, 5, 5}, // 1
    {{2, 0, 8, 16, 32, 0}, 5, 1, 1}, // 2
    {{2, 4, 0, 16, 32, 0}, 9, 5, 5}, // 3
    {{2, 4, 0, 16, 32, 0}, 9, 3, 5}, // 4
    {{2, 0, 8, 0, 32, 0}, 21, 5, 5}, // 5
    {{2, 0, 8, 0, 32, 0}, 21, 4, 5}, // 6
    {{2, 0, 24, 0, 32, 0}, 5, 5, 5}, // 7
    {{2, 0, 24, 0, 32, 0}, 5, 3, 3}, // 8
    {{2, 0, 24, 0, 32, 0}, 5, 1, 1}, // 9
    {{2, 4, 0, 48, 0, 0}, 9, 2, 2}, // 10
    {{2, 4, 0, 48, 0, 0}, 9, 5, 5}, // 11
    {{2, 0, 56, 0, 0, 0}, 5, 1, 1}, // 12
    {{2, 0, 56, 0, 0, 0}, 5, 5, 5}, // 13
    {{2, 0, 8, 48, 0, 0}, 5, 5, 5}, // 14
    {{2, 0, 8, 48, 0, 0}, 5, 1, 1}, // 15
    {{22, 0, 8, 0, 32, 0}, 1, 5, 5}, // 16
    {{22, 0, 8, 0, 32, 0}, 1, 1, 1}, // 17
    {{14, 0, 0, 16, 32, 0}, 1, 2, 2}, // 18
    {{2, 4, 0, 32, 32, 0}, 25, 5, 5}, // 19
    {{4, 4, 0, 48, 0, 0}, 11, 2, 2}, // 20
    {{4, 4, 0, 32, 32, 0}, 27, 2, 2}, // 21
    {{2, 0, 32, 32, 32, 0}, 29, 5, 5}, // 22
    {{0, 4, 0, 32, 32, 0}, 27, 5, 5}, // 23
    {{0, 0, 32, 32, 32, 0}, 31, 5, 5}, // 24
    {{0, 0, 0, 32, 32, 0}, 31, 5, 5}, // 25
    {{0, 4, 0, 48, 0, 0}, 11, 5, 5}, // 26
    {{0, 4, 0, 48, 0, 0}, 11, 2, 2}, // 27
    {{2, 0, 8, 16, 0, 0}, 37, 4, 4}, // 28
    {{0, 0, 8, 0, 32, 0}, 23, 5, 5}, // 29
    {{0, 0, 8, 16, 0, 0}, 39, 4, 4}, // 30
    {{0, 0, 0, 0, 32, 0}, 31, 5, 5}, // 31
    {{0, 0, 0, 0, 0, 0}, 63, 5, 5}, // 32
}};
inline const Graph& get(int algorithm) { return graphs[std::clamp(algorithm,1,32)-1]; }
inline std::array<int,6> levels(const Graph& graph)
{
    std::array<int,6> result {};
    for (int target=0;target<6;++target)
        for (int source=target+1;source<6;++source)
            if (graph.inputs[target] & (1<<source))
                result[source]=std::max(result[source],result[target]+1);
    return result;
}
// Give independent stacks their own columns; fan-in/fan-out groups receive
// enough horizontal space for their widest layer. Values are normalised 0..1.
inline std::array<float,6> columns(const Graph& graph)
{
    std::array<int,6> group {0,1,2,3,4,5};
    for (int target=0;target<6;++target)
        for (int source=target+1;source<6;++source)
            if (graph.inputs[target] & (1<<source))
            {
                const int old=group[source], replacement=group[target];
                for (auto& g:group) if (g==old) g=replacement;
            }
    const auto depth=levels(graph);
    std::array<std::array<int,4>,6> counts {};
    std::array<int,6> widths {};
    for (int op=0;op<6;++op) ++counts[group[op]][depth[op]];
    int total=0;
    for (int g=0;g<6;++g)
    { widths[g]=*std::max_element(counts[g].begin(),counts[g].end()); total+=widths[g]; }
    std::array<float,6> x {};
    int offset=0;
    for (int g=0;g<6;++g)
    {
        std::array<int,4> rank {};
        for (int op=0;op<6;++op)
            if (group[op]==g)
                x[op]=(offset+(rank[depth[op]]++ +0.5f)*widths[g]/counts[g][depth[op]])/total;
        offset+=widths[g];
    }
    return x;
}
}
