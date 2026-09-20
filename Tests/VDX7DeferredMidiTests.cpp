#include "VDX7DeferredMidi.h"
#include "VDX7EditQueue.h"
#include <cstdlib>
#include <iostream>
#include <vector>
#include <thread>

static void checkEditQueue();

void require(bool result) { if (!result) std::exit(1); }
int main()
{
    checkEditQueue();
    VDX7DeferredMidi queue;
    const uint8_t on[] {0x90, 60, 100}, off[] {0x80, 60, 0};
    require(queue.push(on, 3) && queue.push(off, 3));
    std::vector<int> statuses;
    bool panic = false;
    queue.drain([&](const uint8_t* data, std::size_t n)
        { require(n == 3); statuses.push_back(data[0]); }, [&] { panic = true; });
    require(!panic && statuses == std::vector<int>({0x90, 0x80}));
    for (int i = 0; i < 256; ++i) require(queue.push(on, 3));
    require(!queue.push(off, 3));
    require(!queue.push(on, 3));
    statuses.clear();
    queue.drain([&](const uint8_t*, std::size_t) { statuses.push_back(1); },
                [&] { panic = true; });
    require(panic && statuses.empty());
    std::vector<uint8_t> oversized(65537);
    require(!queue.push(oversized.data(), oversized.size()));
    queue.clear();
    require(queue.push(off, 3));
    std::cout << "PASS: deferred event ordering, overflow panic, recovery and byte bounds\n";
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
