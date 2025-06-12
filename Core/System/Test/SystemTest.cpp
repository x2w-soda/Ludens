#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "Extra/doctest/doctest.h"
#include <Ludens/System/Allocator.h>
#include <Ludens/System/Memory.h>

using namespace LD;

TEST_CASE("LinearAllocator")
{
    LinearAllocatorInfo laI{};
    laI.usage = MEMORY_USAGE_MISC;
    laI.capacity = 1024;
    LinearAllocator la = LinearAllocator::create(laI);

    CHECK(la.capacity() == 1024);
    CHECK(la.size() == 0);

    void* p1 = la.allocate(128);

    CHECK(la.size() == 128);
    CHECK(la.remain() == 896);

    void* p2 = la.allocate(128);
    void* p3 = la.allocate(256);

    CHECK(la.size() == 512);
    CHECK(la.remain() == 512);

    la.free();

    CHECK(la.size() == 0);
    CHECK(la.remain() == 1024);

    LinearAllocator::destroy(la);

    const MemoryProfile& profile = get_memory_profile(MEMORY_USAGE_MISC);
    CHECK(profile.current == 0);
}