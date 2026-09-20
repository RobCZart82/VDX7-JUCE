#include "VDX7DeferredMidi.h"
#include "VDX7EditQueue.h"
#include "VDX7KeyboardQueue.h"
#include "VDX7MidiValidation.h"
#include <cstdlib>
#include <iostream>
#include <vector>
#include <thread>

static void checkEditQueue();
static void checkMidiTimeline();
static void checkKeyboardQueue();

void require(bool result) { if (!result) std::exit(1); }
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
                const uint8_t event[] {static_cast<uint8_t>(kind | (channel-1)), 7, 64};
                require(VDX7MidiValidation::acceptsHostEvent(event, kind == 0xc0 || kind == 0xd0 ? 2 : 3, selected)
                        == (selected == 0 || selected == channel));
            }
    checkEditQueue();
    checkMidiTimeline();
    checkKeyboardQueue();
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

static void checkEditQueue()
{
    VDX7EditQueue queue;
    using Kind = VDX7EditQueue::Kind;
    VDX7EditQueue::Command command;
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
