#include "VDX7MonoCorrection.h"
#include <array>
#include <iostream>
#include <stdexcept>

using namespace VDX7MonoCorrection;
static void require(bool value, const char* message)
{
    if (!value) throw std::runtime_error(message);
}

int main()
{
    try
    {
        constexpr std::array<uint16_t, 6> sites {0xd593, 0xd645, 0xd6a8, 0xd6bb, 0xd6ce, 0xd6e0};
        Input in;
        in.enabled = in.profile = in.mono = true;
        in.slotAddress = 0x20b0;
        in.b = 1;
        // All flag bytes and all MIDI keys: key zero has the same ownership
        // semantics as every other key. Non-occupancy bits cannot affect it.
        for (int flags = 0; flags < 256; ++flags)
            for (int key = 0; key < 128; ++key)
                for (int site = 0; site < 6; ++site)
                {
                    in.flags = static_cast<uint8_t>(flags);
                    in.key = in.a = in.requestedKey = static_cast<uint8_t>(key);
                    in.pc = sites[site];
                    const auto d = evaluate(in);
                    const bool occupied = (flags & 2) != 0;
                    require(d.site == site && d.z == (site == 2 ? occupied : !occupied),
                            "shared occupancy contract");
                }
        // Release must also prove successful search and actual key identity.
        in.flags = 2; in.pc = sites[1]; in.key = 0; in.requestedKey = 0;
        in.b = 0; require(evaluate(in).z, "failed search must not release");
        in.b = 1; require(!evaluate(in).z, "found occupied zero must release");
        in.requestedKey = 1; require(evaluate(in).z, "wrong key must not release");
        in.pc = sites[2]; in.a = 1;
        require(!evaluate(in).z, "lookup must compare actual key");
        // All addresses, including odd, end and wrapped values. Invalid table
        // positions cannot be recognized as occupied even if flags claim so.
        for (unsigned address = 0; address < 65536; ++address)
        {
            in.slotAddress = static_cast<uint16_t>(address);
            const bool valid = address >= 0x20b0 && address <= 0x20ce && address % 2 == 0;
            require(validSlot(in.slotAddress) == valid, "slot bounds/alignment");
            in.key = in.a = in.requestedKey = 0;
            for (int site = 0; site < 6; ++site)
            {
                in.pc = sites[site];
                const auto d = evaluate(in);
                if (valid) require(d.site == site, "valid slot recognized");
                else if (site == 1) require(d.z, "invalid release search rejected");
                else if (site == 2) require(!d.z, "invalid lookup rejected");
                else require(d.site == -1 && d.z == in.nativeZ, "invalid pointer untouched");
            }
        }
        // Exhaust instruction addresses and all enable/profile/mode gates,
        // with both native flag values. Disabled paths are exact no-ops.
        in.slotAddress = 0x20b0;
        for (unsigned pc = 0; pc < 65536; ++pc)
            for (int gate = 0; gate < 8; ++gate)
                for (bool z : {false, true})
                {
                    in.pc = static_cast<uint16_t>(pc); in.nativeZ = z;
                    in.enabled = (gate & 1) != 0;
                    in.profile = (gate & 2) != 0;
                    in.mono = (gate & 4) != 0;
                    bool known = false;
                    for (auto site : sites) known |= site == pc;
                    const auto d = evaluate(in);
                    if (gate != 7 || !known)
                        require(d.site == -1 && d.z == z, "native/unknown/POLY path unchanged");
                    else require(d.site >= 0, "enabled known MONO decision");
                }
        std::cout << "PASS: six-site occupancy, release/lookup identity, complete slot/PC bounds and native guards\n";
        return 0;
    }
    catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
