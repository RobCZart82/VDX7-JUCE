#pragma once

#include <cstdint>
#include <limits>
#include <optional>
#include <string_view>

// Test-only contract model. NOT a ValueTree/binary codec, thread-safe mailbox,
// processor transaction or DSP implementation. Callers are serialized in tests.
namespace VDX7SoundModeStatePrototype
{
enum class Mode { classic = 0, clean = 1 };

struct Decoded
{
    bool accepted;
    Mode mode;
};

constexpr Decoded decode(std::optional<std::string_view> version,
                         std::optional<std::string_view> mode) noexcept
{
    if (!version && !mode)
        return { true, Mode::classic }; // Legacy project, even after prior Clean.
    if (!version || !mode || *version != "1" || (*mode != "0" && *mode != "1"))
        return { false, Mode::classic }; // Caller must reject without mutation.
    return { true, *mode == "1" ? Mode::clean : Mode::classic };
}

struct Encoded
{
    std::string_view version, mode;
    bool operator==(const Encoded&) const = default;
};

class Owner
{
public:
    struct Snapshot
    {
        Mode desired;
        std::uint64_t projectRevision;
        bool pending, engineReady;
        bool operator==(const Snapshot&) const = default;
    };

    explicit Owner(std::uint64_t initialRevision = 0) noexcept : revision_(initialRevision) {}

    bool restore(std::optional<std::string_view> version, std::optional<std::string_view> mode,
                 bool compatibleEngineReady) noexcept
    {
        const auto candidate = decode(version, mode);
        // Bounded model guard: never reuse a stale generation after wrap.
        if (!candidate.accepted || revision_ == std::numeric_limits<std::uint64_t>::max())
            return false;
        desired_ = candidate.mode;
        ++revision_;
        ready_ = compatibleEngineReady;
        pending_ = !compatibleEngineReady;
        return true;
    }

    bool requestFromUi(Mode mode, std::uint64_t expectedProjectRevision) noexcept
    {
        if ((mode != Mode::classic && mode != Mode::clean) || expectedProjectRevision != revision_)
            return false;
        desired_ = mode;
        return true;
    }

    Encoded capture() const noexcept
    {
        return { "1", desired_ == Mode::clean ? "1" : "0" };
    }

    // This only models the external ROM-identity transaction result. It does
    // not identify/validate firmware or perform a real pending project restore.
    bool completeCompatibleRestore(std::uint64_t expectedProjectRevision) noexcept
    {
        if (expectedProjectRevision != revision_)
            return false;
        ready_ = true;
        pending_ = false;
        return true;
    }
    void loseEngine() noexcept { ready_ = false; }

    std::optional<Mode> rendererRequest() const noexcept
    {
        if (!ready_ || pending_)
            return std::nullopt;
        return desired_; // Request only: not immediate active-mode application.
    }

    Snapshot snapshot() const noexcept { return { desired_, revision_, pending_, ready_ }; }

private:
    Mode desired_ = Mode::classic;
    std::uint64_t revision_ = 0;
    bool pending_ = false, ready_ = false;
};
} // namespace VDX7SoundModeStatePrototype
