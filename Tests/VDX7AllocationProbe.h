#pragma once
#include <cstdlib>
#include <new>

// Test-executable-only interception of ordinary C++ new/delete on the calling
// thread. This does NOT intercept direct malloc/free or aligned allocations.
namespace VDX7AllocationProbe
{
inline thread_local bool enabled = false;
inline thread_local std::size_t allocations = 0, deallocations = 0;
}
void* operator new(std::size_t size)
{
    if (VDX7AllocationProbe::enabled) ++VDX7AllocationProbe::allocations;
    if (auto* p = std::malloc(size == 0 ? 1 : size)) return p;
    throw std::bad_alloc();
}
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* p) noexcept
{
    if (p && VDX7AllocationProbe::enabled) ++VDX7AllocationProbe::deallocations;
    std::free(p);
}
void operator delete[](void* p) noexcept { ::operator delete(p); }
void operator delete(void* p, std::size_t) noexcept { ::operator delete(p); }
void operator delete[](void* p, std::size_t) noexcept { ::operator delete(p); }
