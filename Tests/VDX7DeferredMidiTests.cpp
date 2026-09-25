#include "VDX7DeferredMidi.h"
#include "VDX7EditQueue.h"
#include "VDX7KeyboardQueue.h"
#include "VDX7MidiValidation.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <thread>

static void checkEditQueue();
static void checkMidiTimeline();
static void checkMidiLagAccounting();
static void checkKeyboardQueue();
static void checkSysExAdmission();

struct VDX7EditQueueTestAccess
{
    template<class Hook>
    static bool pushPaused(VDX7EditQueue& q, VDX7EditQueue::Command c, Hook hook)
    { return q.pushWithReservationHook(c, hook); }
};

void require(bool result) { if (!result) std::exit(1); }
void require(bool result, const char* message)
{
    if (!result)
    {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}
int main()
{
    // Exhaust every status, length and data-byte value without ROM or JUCE.
    require(!VDX7MidiValidation::isChannelMessage(nullptr, 3));
    for (int status = 0; status < 256; ++status)
        for (std::size_t size = 0; size <= 4; ++size)
        {
            uint8_t bytes[] {static_cast<uint8_t>(status), 0, 127, 0};
            const auto kind = status & 0xf0;
            const std::size_t expected = kind == 0xc0 || kind == 0xd0 ? 2 : 3;
            require(VDX7MidiValidation::isChannelMessage(bytes, size)
                    == (status >= 0x80 && status < 0xf0 && size == expected));
            if (status < 0x80 || status >= 0xf0 || size != expected) continue;
            for (std::size_t position = 1; position < size; ++position)
                for (int value = 0; value < 256; ++value)
                {
                    bytes[1] = bytes[2] = 0;
                    bytes[position] = static_cast<uint8_t>(value);
                    require(VDX7MidiValidation::isChannelMessage(bytes, size) == (value < 128));
                }
        }
    std::cout << "PASS: complete channel message validation, all statuses/lengths/data bytes\n";
    for (int selected = 0; selected <= 16; ++selected)
        for (int channel = 1; channel <= 16; ++channel)
            for (int kind = 0x80; kind < 0xf0; kind += 0x10)
            {
                const uint8_t event[] {static_cast<uint8_t>(kind | (channel-1)), 60, 64};
                require(VDX7MidiValidation::acceptsHostEvent(event, kind == 0xc0 || kind == 0xd0 ? 2 : 3, selected)
                        == (selected == 0 || selected == channel));
            }
    for (int note = 0; note < 128; ++note)
        for (const uint8_t status : {uint8_t(0x80), uint8_t(0x90)})
            for (const uint8_t velocity : {uint8_t(0), uint8_t(1), uint8_t(127)})
            {
                const uint8_t event[] {status, static_cast<uint8_t>(note), velocity};
                const bool supported = note >= VDX7MidiValidation::firstSupportedNote
                    && note <= VDX7MidiValidation::lastSupportedNote;
                require(VDX7MidiValidation::acceptsHostEvent(event, sizeof(event), 0) == supported);
            }
    const uint8_t lowNumberedControl[] {0xb0, 11, 127};
    require(VDX7MidiValidation::acceptsHostEvent(lowNumberedControl,
            sizeof(lowNumberedControl), 0), "note-range filter must not reject low-numbered CCs");
    {
        VDX7DeferredMidi filteredRange;
        for (int i = 0; i < 300; ++i)
        {
            const int index = i % 19;
            const auto pitch = static_cast<uint8_t>(index < 12 ? index : 121 + index - 12);
            const uint8_t on[] {0x90, pitch, 100}, off[] {0x80, pitch, 0};
            const bool acceptedOn = VDX7MidiValidation::acceptsHostEvent(on, sizeof(on), 0);
            const bool acceptedOff = VDX7MidiValidation::acceptsHostEvent(off, sizeof(off), 0);
            require(!acceptedOn && !acceptedOff, "excluded pitch tails rejected before deferral");
            if (acceptedOn) require(filteredRange.push(on, sizeof(on)), "queue accepted On");
            if (acceptedOff) require(filteredRange.push(off, sizeof(off)), "queue accepted Off");
        }
        const uint8_t firstSupported[] {0x90, 12, 100};
        require(VDX7MidiValidation::acceptsHostEvent(firstSupported, sizeof(firstSupported), 0)
                && filteredRange.push(firstSupported, sizeof(firstSupported)),
                "filtered-note flood leaves deferred queue capacity for C0");
        bool panic = false;
        int deliveredPitch = -1;
        filteredRange.renderBlock(64, [&](const uint8_t* event, std::size_t size, int) {
            require(size == sizeof(firstSupported));
            deliveredPitch = event[1];
        }, [&] { panic = true; });
        require(!panic && deliveredPitch == 12, "supported lower boundary survives filtered flood");
    }
    checkEditQueue();
    for (int cc = 0; cc < 128; ++cc)
        for (int value = 0; value < 128; ++value)
        {
            const uint8_t event[] {0xb0, static_cast<uint8_t>(cc), static_cast<uint8_t>(value)};
            const bool ignored = cc == 0 || cc == 100 || cc == 101 || (cc == 32 && value >= 8);
            require(VDX7MidiValidation::isChannelMessage(event, 3));
            require(VDX7MidiValidation::isIgnoredAdapterEvent(event, 3) == ignored);
            require(VDX7MidiValidation::acceptsHostEvent(event, 3, 1) == !ignored);
            require(!VDX7MidiValidation::acceptsHostEvent(event, 3, 2));
        }
    checkMidiTimeline();
    checkMidiLagAccounting();
    checkKeyboardQueue();
    checkSysExAdmission();
    VDX7DeferredMidi queue;
    const uint8_t on[] {0x90, 60, 100}, off[] {0x80, 60, 0};
    require(queue.push(on, 3) && queue.push(off, 3));
    std::vector<int> statuses;
    bool panic = false;
    queue.renderBlock(256, [&](const uint8_t* data, std::size_t n, int)
        { require(n == 3); statuses.push_back(data[0]); }, [&] { panic = true; });
    require(!panic && statuses == std::vector<int>({0x90, 0x80}));
    queue.clear();
    for (int i = 0; i < 256; ++i) require(queue.push(on, 3));
    require(!queue.push(off, 3));
    require(!queue.push(on, 3));
    statuses.clear();
    queue.renderBlock(256, [&](const uint8_t*, std::size_t, int) { statuses.push_back(1); },
                [&] { panic = true; });
    require(panic && statuses.empty());
    std::vector<uint8_t> oversized(65537);
    require(!queue.push(oversized.data(), oversized.size()));
    queue.clear();
    require(queue.push(off, 3));
    std::cout << "PASS: deferred event ordering, overflow panic, recovery and byte bounds\n";
}

static void checkSysExAdmission()
{
    const uint8_t note[] {0x90, 60, 100};
    const uint8_t factoryBankSelect[] {0xb0, 32, 3};
    const uint8_t emptySysEx[] {0xf0, 0xf7};

    std::vector<uint8_t> validBank(4104, 0);
    validBank[0] = 0xf0;
    validBank[1] = 0x43;
    validBank[2] = 0x0f; // Device/channel IDs 0..15 are accepted.
    validBank[3] = 0x09;
    validBank[4] = 0x20;
    validBank[5] = 0x00;
    validBank[4103] = 0xf7;
    // A zero payload has a zero checksum at byte 4102.
    require(VDX7MidiValidation::isLiveBankSysex(validBank.data(), validBank.size()));
    require(VDX7MidiValidation::acceptsHostEvent(validBank.data(), validBank.size(), 16));
    require(VDX7MidiValidation::acceptsHostEvent(factoryBankSelect,
                sizeof(factoryBankSelect), 0, true), "factory-bank CC32 remains accepted when available");
    require(!VDX7MidiValidation::acceptsHostEvent(factoryBankSelect,
                sizeof(factoryBankSelect), 0, false), "unavailable factory-bank CC32 rejected before deferral");

    VDX7DeferredMidi unavailableFactoryQueue;
    for (int i = 0; i < 256; ++i)
        if (VDX7MidiValidation::acceptsHostEvent(factoryBankSelect,
                sizeof(factoryBankSelect), 0, false))
            unavailableFactoryQueue.push(factoryBankSelect, sizeof(factoryBankSelect));
    require(unavailableFactoryQueue.push(note, sizeof(note)),
            "unavailable CC32 flood leaves deferred capacity for the following note");
    unavailableFactoryQueue.advanceInputBlock(64, 4096);
    int deliveredNotes = 0;
    bool deferredPanic = false;
    unavailableFactoryQueue.renderBlock(64, [&](const uint8_t* event, std::size_t size, int) {
        require(size == sizeof(note) && event[0] == 0x90 && event[1] == 60,
                "deferred no-factory control delivers the note");
        ++deliveredNotes;
    }, [&] { deferredPanic = true; });
    require(!deferredPanic && deliveredNotes == 1,
            "unavailable CC32 flood does not panic the deferred queue");

    VDX7DeferredMidi unfilteredControl;
    for (int i = 0; i < 256; ++i)
        require(unfilteredControl.push(factoryBankSelect, sizeof(factoryBankSelect)),
                "unfiltered baseline fills the deferred queue");
    require(!unfilteredControl.push(note, sizeof(note)),
            "unfiltered CC32 flood reproduces note eviction at queue capacity");
    unfilteredControl.advanceInputBlock(64, 4096);
    bool baselinePanic = false;
    unfilteredControl.renderBlock(64, [](const uint8_t*, std::size_t, int) {
        require(false, "overflow baseline must discard queued events");
    }, [&] { baselinePanic = true; });
    require(baselinePanic, "unfiltered control triggers the expected queue panic");

    VDX7DeferredMidi availableFactoryQueue;
    for (int i = 0; i < 255; ++i)
        if (VDX7MidiValidation::acceptsHostEvent(factoryBankSelect,
                sizeof(factoryBankSelect), 0, true))
            require(availableFactoryQueue.push(factoryBankSelect, sizeof(factoryBankSelect)),
                    "valid factory-bank CC32 retains deferred capacity");
    require(availableFactoryQueue.push(note, sizeof(note)),
            "valid factory-bank control preserves following note");
    availableFactoryQueue.advanceInputBlock(64, 4096);
    int deliveredEvents = 0;
    availableFactoryQueue.renderBlock(64, [&](const uint8_t*, std::size_t, int) {
        ++deliveredEvents;
    }, [] { require(false, "valid factory-bank queue must not panic"); });
    require(deliveredEvents == 256, "factory-present control retains all admitted events");
    require(!VDX7MidiValidation::isLiveBankSysex(nullptr, validBank.size()));
    require(!VDX7MidiValidation::isLiveBankSysex(validBank.data(), validBank.size() - 1));

    auto corruptBank = validBank;
    corruptBank[4102] = 1;
    require(!VDX7MidiValidation::isLiveBankSysex(corruptBank.data(), corruptBank.size()));
    corruptBank = validBank;
    corruptBank[128] = 0x80;
    require(!VDX7MidiValidation::isLiveBankSysex(corruptBank.data(), corruptBank.size()));

    // Checksum-valid packets with an impossible +8 detune are rejected at the
    // live boundary, in every voice/operator slot, before engine import.
    for (int voice = 0; voice < 32; ++voice)
        for (int op = 0; op < VDX7VoiceData::kOperatorCount; ++op)
        {
            corruptBank = validBank;
            const auto detuneByte = static_cast<std::size_t>(
                6 + voice * VDX7VoiceData::kPackedVoiceSize
                + (VDX7VoiceData::kOperatorCount - 1 - op)
                    * VDX7VoiceData::kPackedOperatorSize + 12);
            corruptBank[detuneByte] = static_cast<uint8_t>((corruptBank[detuneByte] & 0x87) | 0x78);
            int sum = 0;
            for (std::size_t i = 6; i < 4102; ++i) sum += corruptBank[i];
            corruptBank[4102] = static_cast<uint8_t>((128 - (sum & 0x7f)) & 0x7f);
            sum = 0;
            for (std::size_t i = 6; i <= 4102; ++i) sum += corruptBank[i];
            require((sum & 0x7f) == 0, "invalid-detune test packet has valid checksum");
            require(!VDX7MidiValidation::isLiveBankSysex(corruptBank.data(), corruptBank.size()),
                    "live validation rejects invalid detune in every voice/operator slot");
        }

    const auto survivingNote = [&](const uint8_t* invalid, std::size_t invalidSize, int repetitions)
    {
        VDX7DeferredMidi queue;
        for (int n = 0; n < repetitions; ++n)
        {
            require(!VDX7MidiValidation::acceptsHostEvent(invalid, invalidSize, 0));
            // This mirrors processBlock: invalid input is never pushed, so it
            // cannot consume event or byte capacity while rendering is delayed.
        }
        require(VDX7MidiValidation::acceptsHostEvent(note, sizeof(note), 0));
        require(queue.push(note, sizeof(note), 24));
        bool panic = false;
        std::vector<int> statuses, positions;
        queue.renderBlock(64, [&](const uint8_t* data, std::size_t size, int position) {
            require(size == sizeof(note));
            statuses.push_back(data[0]);
            positions.push_back(position);
        }, [&] { panic = true; });
        require(!panic && statuses == std::vector<int>({0x90})
                && positions == std::vector<int>({24}));
    };

    // These are the two capacity-pressure cases reported by the audit.
    survivingNote(emptySysEx, sizeof(emptySysEx), 300);
    corruptBank = validBank;
    corruptBank[4102] = 1;
    survivingNote(corruptBank.data(), corruptBank.size(), 16);

    VDX7DeferredMidi accepted;
    require(accepted.push(validBank.data(), validBank.size()));
    require(accepted.push(note, sizeof(note), 24));
    bool panic = false;
    std::vector<std::size_t> sizes;
    accepted.renderBlock(64, [&](const uint8_t*, std::size_t size, int) { sizes.push_back(size); },
                         [&] { panic = true; });
    require(!panic && sizes == std::vector<std::size_t>({4104, sizeof(note)}));
    std::cout << "PASS: SysEx validation and factory-aware deferred MIDI admission\n";
}

static void checkKeyboardQueue()
{
    VDX7KeyboardQueue q;
    VDX7KeyboardQueue::Event event;
    for (int round = 0; round < 100; ++round)
    {
        for (int i = 0; i < 256; ++i) require(q.push({0x90, static_cast<uint8_t>(i), 100}));
        for (int i = 0; i < 256; ++i)
            require(q.pop(event) && event[1] == static_cast<uint8_t>(i));
        require(!q.pop(event));
    }
    for (int i = 0; i < 256; ++i) require(q.push({0x90, 60, 100}));
    require(!q.push({0x80, 60, 0}));
    require(q.recoverOverflow() && !q.pop(event));
    require(q.push({0x80, 60, 0}) && q.pop(event) && event[0] == 0x80);
    // Producer and consumer overlap, with bounded batches preventing overload.
    std::atomic<int> acknowledged {0};
    std::thread producer([&] {
        for (int batch = 0; batch < 200; ++batch)
        {
            while (acknowledged.load() != batch) std::this_thread::yield();
            for (int i = 0; i < 128; ++i) require(q.push({0x90, static_cast<uint8_t>(i), 100}));
        }
    });
    for (int batch = 0; batch < 200; ++batch)
    {
        for (int i = 0; i < 128; ++i)
        {
            while (!q.pop(event)) std::this_thread::yield();
            require(event[1] == i);
        }
        acknowledged.store(batch + 1);
    }
    producer.join();
    require(!q.recoverOverflow());
    std::cout << "PASS: keyboard queue ordering, ring reuse, overflow recovery and concurrent handoff\n";
}

static void checkMidiTimeline()
{
    VDX7DeferredMidi q;
    const uint8_t on[] {0x90, 60, 100}, off[] {0x80, 60, 0};
    const uint8_t program[] {0xc0, 4}, sustain[] {0xb0, 64, 127};
    require(q.push(on, 3, 16)); q.advanceInputBlock(128, 4096);
    require(q.push(program, 2, 4) && q.push(sustain, 3, 32)); q.advanceInputBlock(128, 4096);
    require(q.push(off, 3, 64)); q.advanceInputBlock(128, 4096);
    std::vector<int> times, statuses;
    for (int start = 0; start < 384; start += 64)
    {
        q.advanceInputBlock(64, 4096);
        q.renderBlock(64, [&](const uint8_t* data, std::size_t, int position) {
            times.push_back(start + position); statuses.push_back(data[0]);
        }, [] { require(false); });
    }
    require(times == std::vector<int>({16, 132, 160, 320}));
    require(statuses == std::vector<int>({0x90, 0xc0, 0xb0, 0x80}));
    q.clear(); times.clear();
    require(q.push(on, 3, 0) && q.push(off, 3, 32)); q.advanceInputBlock(64, 4096);
    for (int start = 0; start < 64; start += 16)
        q.renderBlock(16, [&](const uint8_t*, std::size_t, int pos) { times.push_back(start + pos); },
                      [] { require(false); });
    require(times == std::vector<int>({0, 32}));

    // Future host note-off arrives after the delayed note-on was consumed.
    q.clear(); times.clear();
    require(q.push(on, 3)); q.advanceInputBlock(256, 4096);
    q.advanceInputBlock(64, 4096);
    q.renderBlock(64, [&](const uint8_t*, std::size_t, int p) { times.push_back(p); }, [] { require(false); });
    require(q.push(off, 3, 32)); q.advanceInputBlock(64, 4096);
    for (int start = 64; start < 384; start += 64)
        q.renderBlock(64, [&](const uint8_t*, std::size_t, int p) { times.push_back(start+p); },
                      [] { require(false); });
    require(times == std::vector<int>({0, 352}));

    q.clear(); bool panic = false;
    require(q.push(on, 3)); q.advanceInputBlock(512, 256);
    q.renderBlock(64, [](const uint8_t*, std::size_t, int) { require(false); }, [&] { panic = true; });
    require(panic);
    std::cout << "PASS: deferred sample positions, multiblock ordering, short notes, future offs and lag bound\n";
}

static void checkMidiLagAccounting()
{
    using Playback = VDX7DeferredMidi::Playback;
    const uint8_t on[] {0x90, 72, 100}, off[] {0x80, 72, 0};
    // Same absolute input timeline, differently partitioned successful output.
    // Include same-offset ordering, zero-length callbacks and a future Off.
    for (int rate : {44100, 48000, 96000})
        for (const auto& parts : {std::vector<int>{64}, std::vector<int>{rate * 2},
                                 std::vector<int>{rate * 3}, std::vector<int>{0, 17, 63, 4096, 1, 255}})
        {
            VDX7DeferredMidi q;
            require(q.push(on, 3, 16));
            q.advanceInputBlock(64, rate * 2);
            std::vector<int> times, values;
            int start = 0;
            std::size_t part = 0;
            const int offTime = rate / 2;
            while (start < rate * 3)
            {
                const int n = std::min(parts[part++ % parts.size()], rate * 3 - start);
                if (offTime >= start && offTime < start + n)
                {
                    const uint8_t pedal[] {0xb0, 64, 0};
                    require(q.push(pedal, 3, offTime - start));
                    require(q.push(off, 3, offTime - start));
                }
                q.advanceInputBlock(n, rate * 2, Playback::rendering);
                q.renderBlock(n, [&](const uint8_t* data, std::size_t size, int pos) {
                    require(size == 3 && pos >= 0 && pos <= n);
                    times.push_back(start + pos); values.push_back(data[0]);
                }, [] { require(false); });
                start += n;
            }
            require(times == std::vector<int>({16, offTime + 64, offTime + 64}));
            require(values == std::vector<int>({0x90, 0xb0, 0x80}));
            q.resetIfEmpty(); require(!q.active());
        }
    // Strict boundary: exactly the limit is accepted; one skipped sample more
    // expires it. Neither a zero callback nor a later huge render hides expiry.
    for (int lag : {95999, 96000, 96001, 144000})
        for (int skippedPart : {64, 144000})
        {
            VDX7DeferredMidi q;
            require(q.push(on, 3));
            for (int start = 0; start < lag;)
            {
                const int n = std::min(skippedPart, lag - start);
                q.advanceInputBlock(n, 96000);
                start += n;
            }
            q.advanceInputBlock(0, 96000);
            q.advanceInputBlock(144000, 96000, Playback::rendering);
            int events = 0, panics = 0;
            q.renderBlock(144000, [&](const uint8_t* data, std::size_t size, int pos) {
                require(size == 3 && data[0] == 0x90 && pos == 0); ++events;
            }, [&] { ++panics; });
            require(events == (lag <= 96000 ? 1 : 0) && panics == (lag > 96000 ? 1 : 0));
            if (panics)
            {
                require(q.push(off, 3));
                q.advanceInputBlock(64, 96000, Playback::rendering);
                q.renderBlock(64, [&](const uint8_t* data, std::size_t, int pos) {
                    require(data[0] == 0x80 && pos == 0); ++events;
                }, [] { require(false); });
                require(events == 1);
            }
        }
    std::cout << "PASS: lag bound counts skipped time, not successful block size; boundary/panic recovery preserved\n";
}

static void checkEditQueue()
{
    VDX7EditQueue queue;
    using Kind = VDX7EditQueue::Kind;
    VDX7EditQueue::Command command;
    // A producer reserves before restore/import but publishes afterwards.
    // Edits accepted after the discard boundary must survive, old edits must not.
    require(VDX7EditQueueTestAccess::pushPaused(queue, {Kind::voice, 0, 99}, [&] {
        queue.discard();
        require(queue.push({Kind::voice, 1, 42}));
    }));
    require(queue.pop(command));
    if (command.index != 1 || command.value != 42)
    {
        std::cerr << "FAIL: pre-transaction edit escaped discard boundary\n";
        std::exit(1);
    }
    require(!queue.pop(command) && !queue.pending() && !queue.hasEdits());
    for (auto kind : {Kind::program, Kind::bank, Kind::op, Kind::voice})
    {
        require(VDX7EditQueueTestAccess::pushPaused(queue, {kind, 0, 99}, [&] {
            require(queue.push({Kind::voice, 0, 88}));
            queue.discard();
            // Neither unpublished head nor ready stale successor may escape.
            require(!queue.pop(command));
            require(queue.push({Kind::program, 0, 7}));
            require(queue.push({Kind::voice, 2, 43}));
        }));
        require(queue.pop(command) && command.kind == Kind::program && command.value == 7);
        require(queue.pop(command) && command.kind == Kind::voice && command.value == 43);
        require(!queue.pop(command) && !queue.pending() && !queue.hasEdits());
    }
    // Repeated state replacements while two earlier producers remain paused.
    require(VDX7EditQueueTestAccess::pushPaused(queue, {Kind::voice, 0, 1}, [&] {
        queue.discard();
        require(VDX7EditQueueTestAccess::pushPaused(queue, {Kind::voice, 0, 2}, [&] {
            queue.discard();
            require(queue.push({Kind::voice, 0, 3}));
        }));
    }));
    require(queue.pop(command) && command.value == 3);
    require(!queue.pop(command) && !queue.hasEdits());
    // Saturated queue cannot reuse an unpublished slot. After publication it
    // drains obsolete entries and recovers, without delivering any old command.
    require(VDX7EditQueueTestAccess::pushPaused(queue, {Kind::voice, 0, 1}, [&] {
        for (std::size_t n = 1; n < queue.capacity; ++n)
            require(queue.push({Kind::voice, 0, 2}));
        require(!queue.push({Kind::bank, 0, 4}) && queue.overflowed());
        queue.discard();
        require(!queue.pop(command) && queue.overflowed());
        require(!queue.push({Kind::voice, 0, 3}));
    }));
    require(!queue.pop(command) && !queue.pending() && !queue.hasEdits() && !queue.overflowed());
    require(queue.push({Kind::voice, 0, 4}));
    require(queue.pop(command) && command.value == 4);
    std::cout << "PASS: transaction cutoff, late publishers, repeated discard and delayed overflow recovery\n";
    for (int round = 0; round < 3; ++round)
    {
        for (std::size_t n = 0; n < queue.capacity; ++n)
            require(queue.push({Kind::voice, static_cast<int>(n), round}));
        for (std::size_t n = 0; n < queue.capacity; ++n)
        {
            require(queue.pop(command));
            require(command.index == static_cast<int>(n) && command.value == round);
        }
        require(!queue.pop(command));
    }
    for (std::size_t n = 0; n < queue.capacity; ++n) require(queue.push({Kind::program, 0, 3}));
    require(!queue.push({Kind::program, 0, 4}) && queue.overflowed());
    require(queue.pop(command));
    require(!queue.push({Kind::voice, 0, 7})); // Never edit after a rejected switch.
    queue.discard();
    require(!queue.pending() && !queue.overflowed());

    // Four producers: preserve each producer's ordered sequence and deliver
    // each command once. All writes are bounded below queue capacity.
    std::array<std::thread, 4> producers;
    for (int producer = 0; producer < 4; ++producer)
        producers[producer] = std::thread([&, producer] {
            for (int n = 0; n < 512; ++n) require(queue.push({Kind::voice, producer, n}));
        });
    std::array<int, 4> next {};
    int consumed = 0;
    while (consumed < 2048)
    {
        if (queue.pop(command))
        {
            require(command.value == next[command.index]++);
            ++consumed;
        }
        else std::this_thread::yield();
    }
    for (auto& producer : producers) producer.join();
    require(!queue.pending() && !queue.overflowed());
    std::cout << "PASS: ordered edit queue, wraparound, saturation recovery and concurrent producers\n";
}
