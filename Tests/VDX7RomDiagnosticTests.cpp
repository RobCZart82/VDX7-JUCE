#include "VDX7Engine.h"
#include <iostream>
#include <memory>
#include <stdexcept>
#include <fstream>

static void require(bool condition, const char* message)
{ if (!condition) throw std::runtime_error(message); }
int main(int argc, char** argv)
{
    try
    {
        auto engine = std::make_unique<VDX7Engine>();
        VDX7Engine::RomLoadDiagnostic diagnostic;
        std::vector<uint8_t> image(VDX7Engine::kCombinedRomSize, 0);
        for (int slot : {0, 31, 32, 255})
        {
            image[VDX7Engine::kFirmwareSize + slot * 128 + 12] = 120;
            require(!engine->loadRomImage(image.data(), image.size(), nullptr, 0, &diagnostic), "combined rejected");
            require(diagnostic.code == VDX7Engine::RomLoadDiagnostic::Code::invalidFactoryData
                && diagnostic.voice.voice == static_cast<std::size_t>(slot)
                && diagnostic.voice.byte == 12 && !engine->isLoaded(), "bounded field diagnosis before boot");
            image[VDX7Engine::kFirmwareSize + slot * 128 + 12] = 0;
        }
        require(!engine->loadRomImage(nullptr, image.size(), nullptr, 0, &diagnostic)
            && diagnostic.code == VDX7Engine::RomLoadDiagnostic::Code::invalidInput
            && diagnostic.voice.ok(), "null input clears stale details");
        std::vector<uint8_t> companion(VDX7Engine::kFactoryVoicesSize, 0);
        companion.back() = 128;
        require(!engine->loadRomImage(image.data(), VDX7Engine::kFirmwareSize,
            companion.data(), companion.size(), &diagnostic)
            && diagnostic.code == VDX7Engine::RomLoadDiagnostic::Code::invalidFactoryData
            && diagnostic.voice.voice == 255 && diagnostic.voice.byte == 127,
            "direct companion semantic error");
        require(!engine->loadRomImage(image.data(), VDX7Engine::kFirmwareSize,
            companion.data(), companion.size() - 1, &diagnostic)
            && diagnostic.code == VDX7Engine::RomLoadDiagnostic::Code::invalidInput,
            "partial companion rejected before access");
        if (argc == 2)
        {
            // Opt-in local firmware prefix only; never copies private bank data.
            std::ifstream file(argv[1], std::ios::binary);
            std::vector<uint8_t> firmware(VDX7Engine::kFirmwareSize);
            require(bool(file.read(reinterpret_cast<char*>(firmware.data()), firmware.size())),
                "read local firmware prefix");
            require(engine->loadRomImage(firmware.data(), firmware.size(), nullptr, 0, &diagnostic)
                && diagnostic.code == VDX7Engine::RomLoadDiagnostic::Code::none,
                "valid load clears diagnostic");
            std::vector<uint8_t> before, after;
            require(engine->saveRam(before), "capture loaded RAM");
            const int program = engine->currentProgram(), bank = engine->currentBank();
            const auto voices = engine->factoryVoices();
            image.back() = 128;
            require(!engine->loadRomImage(image.data(), image.size(), nullptr, 0, &diagnostic)
                && diagnostic.code == VDX7Engine::RomLoadDiagnostic::Code::invalidFactoryData,
                "loaded engine rejects malformed combined bank");
            require(engine->isLoaded() && engine->saveRam(after) && before == after
                && program == engine->currentProgram() && bank == engine->currentBank()
                && voices == engine->factoryVoices(), "rejected ROM preserves loaded RAM/catalog/selection");
            require(!engine->loadRomImage(firmware.data(), firmware.size(), companion.data(),
                companion.size(), &diagnostic) && engine->saveRam(after) && before == after,
                "rejected direct companion preserves loaded RAM");
            std::cout << "PASS: local loaded-engine rejection preserves state\n";
        }
        else require(argc == 1, "expected zero or one local firmware argument");
        std::cout << "PASS: direct-engine ROM diagnostics and pre-boot rejection\n";
    }
    catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
