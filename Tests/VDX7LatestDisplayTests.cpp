#include "VDX7LatestDisplay.h"
#include <atomic>
#include <cstdlib>
#include <iostream>
#include <thread>

static void require(bool ok) { if (!ok) std::abort(); }

struct VDX7LatestDisplayTestAccess
{
    template<class Capture, class Hook>
    static void publish(VDX7LatestDisplay& display, Capture capture, Hook hook)
    { display.publishWithHook(capture, hook); }
};

int main()
{
    constexpr uint64_t mask = (uint64_t {1} << 58) - 1;
    // Post-capture writes, same-value writes and arbitrarily many ABA changes.
    // A short wrapping revision counter or value-only CAS would miss these.
    for (int edits : {1, 2, 64, 128, 1024})
    {
        VDX7LatestDisplay display(37);
        VDX7LatestDisplayTestAccess::publish(display, [] { return uint64_t {0}; }, [&] {
            for (int i = 0; i < edits; ++i) display.update(mask, i % 2 ? 37 : 99);
            display.update(mask, 37);
        });
        require(display.read() == 37);
        display.publish([] { return uint64_t {42}; });
        require(display.read() == 42); // Engine-only updates resume after UI quiesces.
    }
    // A UI edit during capture must invalidate publication just like one after.
    VDX7LatestDisplay display;
    display.publish([&] { display.update(mask, 73); return uint64_t {0}; });
    require(display.read() == 73);
    // Per-field updates preserve unrelated fields and hide the internal latch.
    require(display.update(0x7f00, uint64_t {99} << 8) == 73);
    require(display.read() == (uint64_t {99} << 8 | 73));
    for (int value : {-256, -1, 0, 255})
    {
        VDX7LatestDisplay tune(256);
        tune.update(0x1ff, uint64_t(value + 256));
        require(int(tune.read()) - 256 == value);
    }

    // Real threads with exact schedule: pause publisher after capture, finish
    // UI writes without an engine lock, then resume stale commit.
    for (int round = 0; round < 100; ++round)
    {
        VDX7LatestDisplay raced(37);
        std::atomic<bool> captured {false}, uiFinished {false};
        std::thread publisher([&] {
            VDX7LatestDisplayTestAccess::publish(raced, [] { return uint64_t {0}; }, [&] {
                captured.store(true, std::memory_order_release);
                while (!uiFinished.load(std::memory_order_acquire)) std::this_thread::yield();
            });
        });
        while (!captured.load(std::memory_order_acquire)) std::this_thread::yield();
        raced.update(mask, 99);
        raced.update(mask, 37);
        require(raced.read() == 37);
        uiFinished.store(true, std::memory_order_release);
        publisher.join();
        require(raced.read() == 37);
    }
    // Independent UI field writers merge into one coherent payload; engine
    // publication still has one caller and must not acquire a writer lock.
    VDX7LatestDisplay merged;
    std::atomic<int> completed {0};
    auto fieldWriter = [&](int shift) {
        for (int n = 0; n < 10000; ++n) merged.update(uint64_t {0xff} << shift, uint64_t(n & 255) << shift);
        completed.fetch_add(1, std::memory_order_release);
    };
    std::thread a(fieldWriter, 0), b(fieldWriter, 8);
    while (completed.load(std::memory_order_acquire) != 2)
        require((merged.read() & ~uint64_t {0xffff}) == 0);
    a.join(); b.join();
    require(merged.read() == uint64_t(9999 & 255) * 257);
    std::cout << "PASS: bounded publication preserves newer/same/ABA UI edits; coherent reads and concurrent field merges\n";
}
