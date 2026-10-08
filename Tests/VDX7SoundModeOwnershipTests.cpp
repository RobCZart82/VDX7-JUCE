#include "VDX7SoundModeOwner.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <future>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <thread>

using Owner = VDX7SoundModeOwner;
using Mode = Owner::Mode;
namespace Codec = VDX7SoundModeState;
static bool preLockAudio = false, notifyUnderLock = false;
static void require(bool ok, const char* message)
{ if (!ok) throw std::runtime_error(message); }

// Explicit rendezvous, not sleeps or hopes about scheduler order. A watchdog
// converts a missing signal to failure instead of hanging a CI worker forever.
struct Gate
{
    std::promise<void> promise;
    std::shared_future<void> future = promise.get_future().share();
    void signal() { promise.set_value(); }
    void wait() const
    {
        if (future.wait_for(std::chrono::seconds(5)) != std::future_status::ready)
        {
            std::cerr << "FAIL: ownership rendezvous timed out\n";
            std::abort();
        }
        future.get();
    }
};

static juce::ValueTree project(int marker, Mode mode)
{
    juce::ValueTree tree(Codec::rootType);
    tree.setProperty("syntheticProject", marker, nullptr);
    tree.setProperty("syntheticPayload", juce::String(marker) + ": own test bytes", nullptr);
    juce::ValueTree child("SyntheticVoice");
    child.setProperty("value", marker, nullptr);
    tree.addChild(child, -1, nullptr);
    return Codec::writeDetached(tree, mode);
}

// Standalone ownership fixture, NOT a PluginProcessor or firmware/ROM validator.
// It only admits its own tiny synthetic payload. Actual processor RAM, banks,
// mono policy, state epoch and compatible ROM completion remain separate gates.
struct Fixture
{
    std::mutex mutex;
    Owner owner;
    juce::ValueTree payload { Codec::rootType };
    int completedInstalls = 0; // Written only by an owner commit callback.
    explicit Fixture(uint64_t revision = 0) : owner(mutex, revision) {}

    bool recall(const juce::ValueTree& tree, bool ready)
    {
        Mode desired = Mode::classic;
        if (!Codec::read(tree, desired) || !tree["syntheticProject"].isInt()) return false;
        auto detached = tree.createCopy();
        return owner.installValidated(desired, ready, [&]() noexcept { payload = detached; });
    }
    Owner::Capture capture() const
    { return owner.capture([&] { return payload; }); }
    juce::ValueTree save() const
    {
        const auto captured = capture();
        return Codec::writeDetached(captured.payload, captured.mode.desired);
    }
    bool complete(uint64_t revision, bool compatible)
    {
        return owner.completePending(revision, compatible,
            [&]() noexcept { ++completedInstalls; });
    }
};

static juce::ValueTree binaryRoundTrip(const juce::ValueTree& tree)
{
    const auto xml = tree.createXml();
    require(xml != nullptr, "snapshot XML creation");
    juce::MemoryBlock binary;
    juce::AudioProcessor::copyXmlToBinary(*xml, binary);
    const auto restored = juce::AudioProcessor::getXmlFromBinary(binary.getData(), (int) binary.getSize());
    require(restored != nullptr, "snapshot binary decode");
    return juce::ValueTree::fromXml(*restored);
}

static void testAdmissionAndRevision()
{
    Fixture f;
    require(f.recall(project(1, Mode::clean), false), "pending Clean install");
    const auto before = f.capture();
    for (const auto* bad : { "01", "1x", "true" })
    {
        auto tree = project(2, Mode::classic);
        tree.setProperty(Codec::modeProperty, bad, nullptr);
        require(!f.recall(tree, true), "invalid mode rejects whole synthetic recall");
        require(f.owner.snapshot() == before.mode && f.capture().payload.isEquivalentTo(before.payload),
                "invalid recall preserves payload, pending, desired and revision");
    }
    auto legacy = project(3, Mode::clean);
    legacy.removeProperty(Codec::modeProperty, nullptr);
    legacy.removeProperty(Codec::versionProperty, nullptr);
    require(f.recall(legacy, true) && f.owner.snapshot().desired == Mode::classic,
            "legacy recall after Clean installs Classic");
    const auto revision = f.owner.snapshot().revision;
    require(f.recall(legacy, true) && f.owner.snapshot().revision == revision + 1,
            "identical project recall retires old token too");
    bool invalidCommitted = false;
    const auto validState = f.owner.snapshot();
    require(!f.owner.installValidated(static_cast<Mode>(255), true,
                [&]() noexcept { invalidCommitted = true; })
            && !invalidCommitted && f.owner.snapshot() == validState,
            "invalid owner enum cannot invoke payload commit or change revision");
    Fixture last(std::numeric_limits<uint64_t>::max() - 1);
    require(last.recall(project(4, Mode::classic), true)
            && last.owner.snapshot().revision == std::numeric_limits<uint64_t>::max(),
            "last representable revision can be committed without wrap");
    const auto lastSnapshot = last.capture();
    require(!last.recall(project(5, Mode::clean), false)
            && last.owner.snapshot() == lastSnapshot.mode
            && last.capture().payload.isEquivalentTo(lastSnapshot.payload),
            "next recall at exhausted revision preserves the final valid project");
    Fixture exhausted(std::numeric_limits<uint64_t>::max());
    const auto original = exhausted.capture();
    require(!exhausted.recall(project(4, Mode::clean), true), "revision exhaustion cannot wrap");
    require(exhausted.owner.snapshot() == original.mode
            && exhausted.capture().payload.isEquivalentTo(original.payload), "exhaustion never commits payload");
}

static void testBusyAndIsolation()
{
    Fixture f, other;
    require(f.recall(project(10, Mode::classic), true), "busy fixture ready");
    const auto before = f.owner.snapshot();
    Owner::Request request = Owner::Request::accepted;
    Owner::Audio audio = Owner::Audio::visited;
    int notifications = 0, visits = 0;
    {
        const std::lock_guard lock(f.mutex);
        std::jthread ui([&] { request = f.owner.tryRequest(Mode::clean, before.revision, [&] { ++notifications; }); });
        std::jthread render([&] { audio = f.owner.tryWithAudioOwner([&](Mode) noexcept { ++visits; }); });
        ui.join(); render.join(); // Both must finish while another thread owns the mutex.
        require(other.owner.tryRequest(Mode::clean, 0, [] {}) == Owner::Request::accepted,
                "a separate instance does not inherit another instance's lock");
    }
    require(request == Owner::Request::busy && audio == Owner::Audio::busy && visits == 0 && notifications == 0,
            "contended UI/audio do not wait, notify, dispatch or queue");
    require(f.owner.snapshot() == before && other.owner.snapshot().desired == Mode::clean,
            "busy request is not replayed later and instances stay isolated");
    require(f.owner.tryRequest(static_cast<Mode>(255), before.revision, [] {}) == Owner::Request::invalid,
            "invalid enum is rejected");
    require(f.owner.tryRequest(Mode::classic, before.revision, [&] { ++notifications; }) == Owner::Request::unchanged
            && notifications == 0, "unchanged request sends no dirty notification");
}

static void testAudioPostLockAndProtectedPayload()
{
    Fixture f;
    require(f.recall(project(20, Mode::classic), true), "audio fixture ready");
    Gate capturedBeforeLock, allowAudio;
    Mode observed = Mode::classic;
    int marker = 0;
    Owner::Audio result = Owner::Audio::unavailable;
    std::jthread audio([&]
    {
        const auto stale = f.owner.snapshot().desired;
        capturedBeforeLock.signal();
        allowAudio.wait();
        result = f.owner.tryWithAudioOwner([&](Mode current) noexcept
        {
            observed = preLockAudio ? stale : current; // Broken opt-in test adapter.
            marker = (int) f.payload["syntheticProject"];
        });
    });
    capturedBeforeLock.wait();
    require(f.recall(project(21, Mode::clean), true), "new project installs before audio ownership");
    allowAudio.signal(); audio.join();
    require(result == Owner::Audio::visited && observed == Mode::clean && marker == 21,
            "audio must use current project/mode after acquiring ownership, never pre-lock mode");

    Gate insideAudio, leaveAudio;
    bool contenderGotLock = true;
    std::jthread heldAudio([&]
    {
        f.owner.tryWithAudioOwner([&](Mode) noexcept { insideAudio.signal(); leaveAudio.wait(); });
    });
    insideAudio.wait();
    std::jthread contender([&]
    {
        const std::unique_lock lock(f.mutex, std::try_to_lock);
        contenderGotLock = lock.owns_lock();
    });
    contender.join(); leaveAudio.signal(); heldAudio.join();
    require(!contenderGotLock, "renderer callback keeps the same payload ownership throughout its visit");
}

static void testStaleUIAndCompletion()
{
    Fixture f;
    require(f.recall(project(30, Mode::clean), false), "first pending project");
    const auto old = f.owner.snapshot();
    Gate uiCaptured, allowUI, completionCaptured, allowCompletion;
    Owner::Request request = Owner::Request::accepted;
    bool completed = true;
    std::jthread ui([&]
    {
        uiCaptured.signal(); allowUI.wait();
        request = f.owner.tryRequest(Mode::clean, old.revision, [] {});
    });
    std::jthread completion([&]
    {
        completionCaptured.signal(); allowCompletion.wait();
        completed = f.complete(old.revision, true);
    });
    uiCaptured.wait(); completionCaptured.wait();
    require(f.recall(project(31, Mode::classic), false), "second pending project supersedes both old operations");
    const auto latest = f.capture();
    // Serialize these two rejection probes so UI try-lock cannot legitimately
    // return BUSY because the unrelated completion probe owns the mutex.
    allowUI.signal(); ui.join();
    allowCompletion.signal(); completion.join();
    require(request == Owner::Request::stale && !completed && f.completedInstalls == 0,
            "old UI and old compatible completion cannot mutate new pending project");
    require(f.owner.snapshot() == latest.mode && f.capture().payload.isEquivalentTo(latest.payload),
            "stale operations preserve entire captured project");
    require(f.owner.tryRequest(Mode::clean, latest.mode.revision, [] {}) == Owner::Request::accepted,
            "fresh pending UI edit is accepted");
    int calls = 0;
    require(f.owner.tryWithAudioOwner([&](Mode) noexcept { ++calls; }) == Owner::Audio::unavailable && calls == 0,
            "pending mode never dispatches onto unrelated old renderer");
    require(!f.complete(latest.mode.revision, false), "mismatched firmware cannot complete pending project");
    require(f.complete(latest.mode.revision, true) && !f.complete(latest.mode.revision, true),
            "compatible current completion commits once");
    require(f.owner.snapshot().desired == Mode::clean && f.owner.snapshot().ready,
            "completion preserves latest pending edit, not saved stale mode");
}

static void testDetachedConcurrentSave()
{
    Fixture f;
    require(f.recall(project(40, Mode::classic), false), "save fixture pending");
    const auto revision = f.owner.snapshot().revision;
    require(f.owner.tryRequest(Mode::clean, revision, [] {}) == Owner::Request::accepted, "pending desired edit");
    Gate detached, encodeNow;
    juce::ValueTree saved;
    std::jthread saving([&]
    {
        const auto captured = f.capture();
        detached.signal(); encodeNow.wait();
        saved = binaryRoundTrip(Codec::writeDetached(captured.payload, captured.mode.desired));
    });
    detached.wait();
    {
        const std::lock_guard lock(f.mutex);
        // Mutate the same shared child, not just replace the root handle: a
        // shallow capture would now silently change the pending save's voice.
        f.payload.getChild(0).setProperty("value", 400, nullptr);
    }
    require(f.recall(project(41, Mode::classic), true), "recall changes live payload while old snapshot awaits encoding");
    encodeNow.signal(); saving.join();
    Mode restored = Mode::classic;
    require(Codec::read(saved, restored) && restored == Mode::clean
            && saved["syntheticProject"].toString() == "40"
            && saved.getChild(0)["value"].toString() == "40",
            "detached binary save contains coherent old payload plus latest accepted pending desired");
    require(saved.getNumProperties() == 4 && saved.getNumChildren() == 1,
            "save contains no runtime revision/pending/ready/gain tokens");
    require(f.owner.snapshot().desired == Mode::classic && (int) f.capture().payload["syntheticProject"] == 41,
            "encoding old captured project cannot mutate new live project");
}

static void testNotificationOutsideLockAndReentrantRecall()
{
    Fixture f;
    require(f.recall(project(50, Mode::classic), true), "notification fixture ready");
    const auto revision = f.owner.snapshot().revision;
    bool lockWasFree = false;
    const auto probe = [&]
    {
        std::jthread thread([&]
        {
            const std::unique_lock lock(f.mutex, std::try_to_lock);
            lockWasFree = lock.owns_lock();
        });
        thread.join();
    };
    if (notifyUnderLock)
    {
        f.owner.tryRequest(Mode::clean, revision, [] {});
        const std::lock_guard lock(f.mutex);
        probe(); // Deliberately broken adapter; no recursive mutex attempt.
    }
    else require(f.owner.tryRequest(Mode::clean, revision, probe) == Owner::Request::accepted, "notify accepted edit");
    require(lockWasFree, "host notification must execute outside project/engine ownership");

    juce::ValueTree saved;
    bool recalled = false;
    require(f.owner.tryRequest(Mode::classic, revision, [&]
    {
        saved = f.save(); // Real same-thread reentrant capture, not a serial-model assertion.
        recalled = f.recall(project(51, Mode::clean), false);
    }) == Owner::Request::accepted, "notification can synchronously save and recall");
    Mode desired = Mode::clean;
    require(Codec::read(saved, desired) && desired == Mode::classic && recalled,
            "reentrant save observes the accepted desired value");
    require(f.owner.snapshot().desired == Mode::clean && f.owner.snapshot().pending
            && (int) f.capture().payload["syntheticProject"] == 51,
            "request never overwrites a newer recall after the notification returns");
}

int main(int argc, char** argv)
{
    try
    {
        if (argc == 2 && std::string_view(argv[1]) == "--pre-lock-audio-negative-control") preLockAudio = true;
        else if (argc == 2 && std::string_view(argv[1]) == "--notify-under-lock-negative-control") notifyUnderLock = true;
        else require(argc == 1, "unknown ownership test argument");
        testAdmissionAndRevision();
        testBusyAndIsolation();
        testAudioPostLockAndProtectedPayload();
        testStaleUIAndCompletion();
        testDetachedConcurrentSave();
        testNotificationOutsideLockAndReentrantRecall();
        std::cout << "PASS: standalone sound-mode owner, real mutex/thread rendezvous and reentrant detached save\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
