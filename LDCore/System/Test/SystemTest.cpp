#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "Extra/doctest/doctest.h"
#include <Ludens/System/Allocator.h>
#include <Ludens/System/Memory.h>
#include <unordered_set>
#include <vector>

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

TEST_CASE("PoolAllocator single page")
{
    PoolAllocatorInfo paI{};
    paI.blockSize = sizeof(int);
    paI.isMultiPage = false;
    paI.pageSize = 4;
    paI.usage = MEMORY_USAGE_MISC;
    PoolAllocator pa = PoolAllocator::create(paI);

    CHECK(pa.page_count() == 0);

    int* i0 = (int*)pa.allocate();

    CHECK(pa.page_count() == 1);

    int* i1 = (int*)pa.allocate();
    int* i2 = (int*)pa.allocate();
    int* i3 = (int*)pa.allocate();

    *i0 = 0;
    *i1 = 1;
    *i2 = 2;
    *i3 = 3;

    // single page mode runes out of blocks
    CHECK(pa.allocate() == nullptr);

    // free blocks in any order
    pa.free(i1);

    // this must be i1
    i1 = (int*)pa.allocate();

    // allocator does not touch user region
    CHECK(*i1 == 1);

    // out of blocks
    CHECK(pa.allocate() == nullptr);

    PoolAllocator::destroy(pa);

    const MemoryProfile& profile = get_memory_profile(MEMORY_USAGE_MISC);
    CHECK(profile.current == 0);
}

TEST_CASE("PoolAllocator multi page")
{
    PoolAllocatorInfo paI{};
    paI.blockSize = sizeof(size_t);
    paI.isMultiPage = true;
    paI.pageSize = 1;
    paI.usage = MEMORY_USAGE_MISC;
    PoolAllocator pa = PoolAllocator::create(paI);

    CHECK(pa.page_count() == 0);

    const size_t N = 10;
    std::vector<size_t*> v(N);

    for (size_t i = 0; i < N; i++)
    {
        v[i] = (size_t*)pa.allocate();
        *v[i] = i;

        CHECK(pa.page_count() == i + 1);
    }

    // free in arbitrary order
    for (auto ite = v.rbegin(); ite != v.rend(); ite++)
        pa.free(*ite);

    // pages are not gone
    CHECK(pa.page_count() == N);

    PoolAllocator::destroy(pa);

    const MemoryProfile& profile = get_memory_profile(MEMORY_USAGE_MISC);
    CHECK(profile.current == 0);
}

template <size_t N, size_t PageSize>
void test_pool_allocator_iterator()
{
    PoolAllocatorInfo paI{};
    paI.blockSize = sizeof(int);
    paI.isMultiPage = true;
    paI.pageSize = PageSize;
    paI.usage = MEMORY_USAGE_MISC;
    PoolAllocator pa = PoolAllocator::create(paI);

    std::unordered_set<int> set;

    for (int i = 0; i < N; i++)
    {
        int* p = (int*)pa.allocate();
        *p = i;
        set.insert(i);
    }

    int ctr = 0;
    for (auto ite = pa.begin(); ite; ++ite)
    {
        set.erase(*(int*)ite.data());
        ctr++;
    }

    CHECK(ctr == N);
    CHECK(set.empty());

    PoolAllocator::destroy(pa);
}

TEST_CASE("PoolAllocator iterator")
{
    test_pool_allocator_iterator<0, 1>();
    test_pool_allocator_iterator<8, 1>();
    test_pool_allocator_iterator<8, 127>();
    test_pool_allocator_iterator<128, 128>();
    test_pool_allocator_iterator<513, 128>();

    const MemoryProfile& profile = get_memory_profile(MEMORY_USAGE_MISC);
    CHECK(profile.current == 0);
}