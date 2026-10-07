#include "VDX7Engine.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>

static void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
int main(int argc, char** argv)
{
    try
    {
        require(argc == 2, "provide one private firmware file");
        std::ifstream file(argv[1], std::ios::binary);
        std::vector<uint8_t> firmware(VDX7Engine::kFirmwareSize);
        require(bool(file.read(reinterpret_cast<char*>(firmware.data()), firmware.size())), "read firmware prefix");
        // Original synthetic single-carrier patch, not copied factory voice data.
        std::vector<uint8_t> banks(VDX7Engine::kFactoryVoicesSize, 0);
        for (std::size_t v = 0; v < banks.size(); v += 128)
        {
            for (int op = 0; op < 6; ++op)
            {
                const auto base = v + 17 * op;
                for (int i = 0; i < 3; ++i) { banks[base+i] = 99; banks[base+4+i] = 99; }
                banks[base+3] = 80;
                banks[base+12] = 7 << 3;
                banks[base+14] = op == 5 ? 99 : 0;
                banks[base+15] = 1 << 1;
            }
            banks[v+110] = 31;
            banks[v+117] = 24;
            std::fill_n(banks.begin()+v+118, 10, uint8_t(' '));
        }
        auto engine = std::make_unique<VDX7Engine>();
        require(engine->loadRomImage(firmware.data(), firmware.size(), banks.data(), banks.size()), "load firmware and synthetic patch");
        engine->prepare(48000);
        std::vector<float> left(48000), right(48000);
        engine->render(left.data(), right.data(), 48000); // settle boot/patch queues
        const uint8_t on[] {0x90, 60, 100}, off[] {0x80, 60, 0};
        engine->handleMidi(on, 3);
        engine->render(left.data(), right.data(), 48000);
        double peak = 0;
        for (float sample : left) { require(std::isfinite(sample), "nonfinite note output"); peak = std::max(peak, double(std::abs(sample))); }
        require(peak > 0.0001, "no measurable note output");
        engine->handleMidi(off, 3);
        for (int i = 0; i < 3; ++i) engine->render(left.data(), right.data(), 48000);
        double tail = 0;
        for (float sample : left) { require(std::isfinite(sample), "nonfinite release output"); tail = std::max(tail, double(std::abs(sample))); }
        require(tail < 0.0001, "note-off fails to retire synthetic patch");
        std::cout << "PASS: synthetic MIDI note/release at 48kHz; peak=" << peak << " tail=" << tail << '\n';
    }
    catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
