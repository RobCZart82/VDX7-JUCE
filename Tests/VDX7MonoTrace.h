#pragma once

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

// Test-only, read-only observer of the validated v1.8 image. No hooks, ROM/RAM
// patches or altered CPU semantics. Never used by the production processor.
struct VDX7MonoTrace
{
    struct State
    {
        uint64_t cycle;
        uint16_t pc, x, entry;
        uint8_t a, b, ccr, note, count;
    };
    struct Step
    {
        State before, after;
        uint8_t opcode;
        const char* op;
        uint16_t address;
        bool write;
        uint8_t value;
    };
    bool enabled = false;
    std::vector<Step> steps;

    static uint16_t word(const dx7Emu::DX7& d, uint16_t address)
    { return uint16_t((d.memory[address] << 8) | d.memory[address + 1]); }

    static State state(const dx7Emu::DX7& d)
    {
        const bool slot = d.IX >= 0x20b0 && d.IX < 0x20d0 && (d.IX & 1) == 0;
        return {d.cycle, d.PC, d.IX, slot ? word(d, d.IX) : uint16_t(0xffff),
                d.A, d.B, uint8_t((d.H << 5) | (d.I << 4) | (d.N << 3)
                                  | (d.Z << 2) | (d.V << 1) | d.C),
                d.memory[0x81], d.memory[0x8e]};
    }

    void start() { steps.clear(); steps.reserve(4096); enabled = true; }
    void stop() { enabled = false; }
    void observe(const State& before, const dx7Emu::DX7& d)
    {
        if (!enabled) return;
        const bool relevant = before.pc >= 0xd583 && before.pc <= 0xd6b3;
        const bool keyEvent = d.inst->w && d.ADDR == 0x30f1;
        if (!relevant && !keyEvent) return;
        if (steps.size() == 4096) throw std::runtime_error("bounded MONO trace overflow");
        steps.push_back({before, state(d), d.opcode, d.inst->op, d.ADDR,
                         d.inst->w, d.memory[d.ADDR]});
    }

    static void verifyImage(const dx7Emu::DX7& d)
    {
        // Match decoded instructions AND operands to the pinned annotated
        // source (ajxs commit 879a25e), without embedding/dumping a ROM image.
        struct Site { uint16_t pc; const char* op; const char* mode; int bytes, operand; };
        const Site sites[] {
            {0xd583, "clr ", "ex", 3, 0x91},
            {0xd586, "ldx ", "im", 3, 0x20b0},
            {0xd589, "ldab", "di", 2, 0x8e},
            {0xd58b, "cmpb", "im", 2, 16},
            {0xd58d, "beq ", "im", 2, 0xd5f1 - 0xd58f},
            {0xd58f, "ldab", "im", 2, 16},
            {0xd591, "tst ", "in", 2, 0},
            {0xd593, "beq ", "im", 2, 0xd59b - 0xd595},
            {0xd59b, "ldaa", "di", 2, 0x81},
            {0xd59d, "ldab", "im", 2, 2},
            {0xd59f, "std ", "in", 2, 0},
            {0xd5a1, "inc ", "ex", 3, 0x8e},
            {0xd5f1, "rts ", "id", 1, 0},
            {0xd637, "ldaa", "di", 2, 0x81},
            {0xd639, "staa", "di", 2, 0x90},
            {0xd63b, "ldd ", "di", 2, 0x9d},
            {0xd63d, "std ", "ex", 3, 0x20d0},
            {0xd641, "jsr ", "ex", 3, 0xd69f},
            {0xd644, "tsta", "id", 1, 0},
            {0xd645, "beq ", "im", 2, 0xd666 - 0xd647},
            {0xd647, "ldd ", "im", 3, 0},
            {0xd64a, "std ", "in", 2, 0},
            {0xd651, "dec ", "ex", 3, 0x8e},
            {0xd661, "ldab", "im", 2, 2},
            {0xd663, "stab", "ex", 3, 0x30f1},
            {0xd666, "rts ", "id", 1, 0},
            {0xd69f, "ldab", "im", 2, 16},
            {0xd6a1, "ldx ", "im", 3, 0x20b0},
            {0xd6a4, "ldaa", "di", 2, 0x81},
            {0xd6a6, "cmpa", "in", 2, 0},
            {0xd6a8, "beq ", "im", 2, 0xd6b1 - 0xd6aa},
            {0xd6af, "clra", "id", 1, 0},
            {0xd6b1, "ldaa", "in", 2, 0},
            {0xd6b3, "rts ", "id", 1, 0}
        };
        for (const auto& s : sites)
        {
            const auto& i = d.instructions[d.memory[s.pc]];
            const int operand = s.bytes == 3 ? word(d, s.pc + 1)
                              : s.bytes == 2 ? d.memory[s.pc + 1] : 0;
            if (std::strcmp(i.op, s.op) || i.mode == nullptr || std::strcmp(i.mode, s.mode)
                || i.bytes != s.bytes || operand != s.operand)
                throw std::runtime_error("local ROM differs from MONO trace instruction map at "
                                         + std::to_string(s.pc));
        }
    }

    int visits(uint16_t pc) const
    {
        return int(std::count_if(steps.begin(), steps.end(),
                                [pc](const Step& s) { return s.before.pc == pc; }));
    }
    const Step& at(uint16_t pc, int occurrence = 0) const
    {
        for (const auto& s : steps)
            if (s.before.pc == pc && occurrence-- == 0) return s;
        throw std::runtime_error("required MONO trace site not executed: " + std::to_string(pc));
    }
    int keyOffWrites() const
    {
        return int(std::count_if(steps.begin(), steps.end(), [](const Step& s)
            { return s.write && s.address == 0x30f1 && (s.value & 3) == 2; }));
    }
    void print(const char* label) const
    {
        std::cout << "TRACE " << label << " (read-only; not a fix)\n";
        for (const auto& s : steps)
        {
            const auto pc = s.before.pc;
            if (pc != 0xd591 && pc != 0xd593 && pc != 0xd59f && pc != 0xd5a1
                && pc != 0xd58d && pc != 0xd6b1 && pc != 0xd644 && pc != 0xd645
                && pc != 0xd64a && pc != 0xd651 && !(s.write && s.address == 0x30f1)) continue;
            std::cout << "cycle=" << s.before.cycle << " pc=" << std::hex << pc
                      << " opcode=" << int(s.opcode) << " " << s.op
                      << " A=" << int(s.before.a) << "->" << int(s.after.a)
                      << " B=" << int(s.before.b) << "->" << int(s.after.b)
                      << " X=" << s.before.x << "->" << s.after.x
                      << " CCR=" << int(s.before.ccr) << "->" << int(s.after.ccr)
                      << " entry=" << s.before.entry << "->" << s.after.entry
                      << " nextPC=" << s.after.pc << std::dec
                      << " note=" << int(s.before.note) << " count=" << int(s.before.count)
                      << "->" << int(s.after.count) << '\n';
        }
    }
};
