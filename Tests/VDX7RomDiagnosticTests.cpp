#include "VDX7Engine.h"
#include <iostream>
#include <memory>
#include <stdexcept>

static void require(bool condition, const char* message)
{ if (!condition) throw std::runtime_error(message); }
int main()
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
        std::cout << "PASS: direct-engine ROM diagnostics and pre-boot rejection\n";
    }
    catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
