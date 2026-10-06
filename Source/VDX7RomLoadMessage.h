#pragma once

#include "VDX7Engine.h"
#include "VDX7ValidationMessage.h"

// Non-RT presentation only; never include firmware bytes or file paths.
inline std::string vdx7RomLoadMessage(const VDX7Engine::RomLoadDiagnostic& diagnostic)
{
    using Code = VDX7Engine::RomLoadDiagnostic::Code;
    switch (diagnostic.code)
    {
        case Code::none: return {};
        case Code::invalidInput:
            return "Invalid ROM input: expected 16384-byte firmware or 49152-byte combined image, with an optional 32768-byte factory image.";
        case Code::invalidFactoryData:
            return "Invalid ROM factory data: " + vdx7ValidationMessage(diagnostic.voice);
        case Code::firmwareRejected:
            return "ROM firmware was rejected by the engine.";
        case Code::bootFailed:
            return "ROM firmware could not be started by the engine.";
    }
    return "ROM load failed: unknown engine diagnostic.";
}
