#pragma once

#include "VDX7SoundModeState.h"
#include <cstdint>
#include <limits>
#include <mutex>
#include <type_traits>

// Preparatory component, NOT connected to PluginProcessor/DSP yet. The caller's
// existing engine mutex must outlive this owner and protect the payload too.
// Call these entry points without already holding that mutex. Payload callbacks
// execute under it and must not reenter the owner, notify a host, or do file I/O.
// Full project/ROM admission remains the caller's job; the D5 codec is not that
// admission. No Clean metadata may be shipped before renderer integration.
class VDX7SoundModeOwner
{
public:
    using Mode = VDX7SoundModeState::Mode;
    struct Snapshot
    {
        Mode desired = Mode::classic;
        uint64_t revision = 0;
        bool pending = false;
        bool ready = false;
        bool operator==(const Snapshot&) const = default;
    };
    struct Capture { juce::ValueTree payload; Snapshot mode; };
    enum class Request { accepted, unchanged, busy, stale, invalid };
    enum class Audio { visited, busy, unavailable };

    explicit VDX7SoundModeOwner(std::mutex& engineMutex, uint64_t initialRevision = 0)
        : mutex_(engineMutex) { state_.revision = initialRevision; }
    VDX7SoundModeOwner(const VDX7SoundModeOwner&) = delete;
    VDX7SoundModeOwner& operator=(const VDX7SoundModeOwner&) = delete;

    Snapshot snapshot() const
    {
        const std::lock_guard lock(mutex_);
        return state_;
    }

    // Notification is deliberately outside ownership; it may synchronously
    // save or recall another project. Never write state after that callback.
    template<class Notify>
    Request tryRequest(Mode desired, uint64_t revision, Notify&& notify)
    {
        if (!VDX7SoundModeState::isValid(desired)) return Request::invalid;
        std::unique_lock lock(mutex_, std::try_to_lock);
        if (!lock.owns_lock()) return Request::busy;
        if (revision != state_.revision) return Request::stale;
        if (desired == state_.desired) return Request::unchanged;
        state_.desired = desired;
        lock.unlock();
        notify();
        return Request::accepted;
    }

    // Non-audio. Only call AFTER validation of the complete candidate project.
    // A nonthrowing payload commit and the mode/revision form one transaction.
    // Even identical recalls get a new revision; exhaustion never wraps.
    template<class Commit>
    bool installValidated(Mode desired, bool engineReady, Commit&& commit)
    {
        static_assert(std::is_nothrow_invocable_v<Commit>);
        if (!VDX7SoundModeState::isValid(desired)) return false;
        const std::lock_guard lock(mutex_);
        if (state_.revision == std::numeric_limits<uint64_t>::max()) return false;
        commit();
        state_ = { desired, state_.revision + 1, !engineReady, engineReady };
        return true;
    }

    // Non-audio. Compatibility must come from the real ROM admission, not from
    // the mode value. Do not read an old desired value from the pending tree.
    template<class Commit>
    bool completePending(uint64_t revision, bool compatible, Commit&& commit)
    {
        static_assert(std::is_nothrow_invocable_v<Commit>);
        const std::lock_guard lock(mutex_);
        if (!compatible || revision != state_.revision || !state_.pending) return false;
        commit();
        state_.pending = false;
        state_.ready = true;
        return true;
    }

    // Non-audio: clone under ownership; XML/binary encoding belongs outside.
    // The capture callback can choose live or pending payload, but it must use
    // the same mutex for all payload writes. No shared ValueTree escapes.
    template<class ReadPayload>
    Capture capture(ReadPayload&& readPayload) const
    {
        const std::lock_guard lock(mutex_);
        return { readPayload().createCopy(), state_ };
    }

    // One existing try-lock, scalar state only, no new queue/wait/XML/hash.
    // The caller's nonthrowing renderer callback remains under ownership.
    // Pending must not dispatch a request onto an unrelated old engine.
    template<class Visit>
    Audio tryWithAudioOwner(Visit&& visit)
    {
        static_assert(std::is_nothrow_invocable_v<Visit, Mode>);
        const std::unique_lock lock(mutex_, std::try_to_lock);
        if (!lock.owns_lock()) return Audio::busy;
        if (!state_.ready || state_.pending) return Audio::unavailable;
        visit(state_.desired);
        return Audio::visited;
    }

private:
    std::mutex& mutex_;
    Snapshot state_;
};
