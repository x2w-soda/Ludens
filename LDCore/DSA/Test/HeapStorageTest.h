#pragma once

#include <Extra/doctest/doctest.h>
#include <Ludens/DSA/HeapStorage.h>
#include <Ludens/DSA/Optional.h>
#include <Ludens/Memory/Memory.h>
#include <type_traits>

using namespace LD;

template <typename T, size_t N>
void test_heap_storage_ctor()
{
    if constexpr (std::is_same<T, Foo>::value)
        Foo::reset();

    {
        THeapStorage<T> s1;
        CHECK(s1.data == nullptr);
        CHECK(s1.size == 0);
        CHECK(s1.cap == 0);

        THeapStorage<T> s2(N);
        CHECK(s2.data != nullptr);
        CHECK(s2.size == 0);
        CHECK(s2.cap == N);

        s2.resize(N); // constructs N elements
        CHECK(s2.size == N);

        for (size_t i = 0; i < N; i++)
            s2.data[i] = (int)i;

        s2.grow(2 * N); // move constructs N, destructs N
        CHECK(s2.cap == 2 * N);
        CHECK(s2.size == N);

        for (size_t i = 0; i < N; i++)
            CHECK(s2.data[i] == (int)i);

        s2.release(); // destructs N
        CHECK(s2.data == nullptr);
        CHECK(s2.cap == 0);
    }
}

TEST_CASE("HeapStorage ctor")
{
    Foo::reset();

    constexpr size_t N = 4;
    test_heap_storage_ctor<Foo, N>();
    test_heap_storage_ctor<int, N>();
    test_heap_storage_ctor<std::optional<int>, N>();

    const MemoryProfile& profile = get_memory_profile(MEMORY_USAGE_MISC);
    CHECK(profile.current == 0);
    CHECK(Foo::sCtor == N);
    CHECK(Foo::sDtor == 2 * N);
    CHECK(Foo::sCopyAssign == 0);
    CHECK(Foo::sCopyCtor == 0);
    CHECK(Foo::sMoveAssign == 0);
    CHECK(Foo::sMoveCtor == N);
}

template <typename T, size_t N>
void test_heap_storage_copy()
{
    THeapStorage<T> s1(N);
    s1.resize(N); // construct N
    CHECK(s1.size == N);
    CHECK(s1.cap == N);

    for (size_t i = 0; i < N; i++)
        s1.data[i] = (int)i;

    THeapStorage<T> s2(s1); // copy construct N
    CHECK(s2.size == N);
    CHECK(s2.cap == N);

    THeapStorage<T> s3;
    s3 = s1; // copy assign N
    CHECK(s3.size == N);
    CHECK(s3.cap == N);

    for (size_t i = 0; i < N; i++)
    {
        CHECK(s2.data[i] == (int)i);
        CHECK(s3.data[i] == (int)i);
    }

    // destruct 3 * N
}

TEST_CASE("HeapStorage copy")
{
    Foo::reset();

    constexpr size_t N = 4;
    test_heap_storage_copy<Foo, N>();
    test_heap_storage_copy<int, N>();
    test_heap_storage_copy<std::optional<int>, N>();

    const MemoryProfile& profile = get_memory_profile(MEMORY_USAGE_MISC);
    CHECK(profile.current == 0);
    CHECK(Foo::sCtor == N);
    CHECK(Foo::sDtor == 3 * N);
    CHECK(Foo::sCopyAssign == N);
    CHECK(Foo::sCopyCtor == N);
    CHECK(Foo::sMoveAssign == 0);
    CHECK(Foo::sMoveCtor == 0);
}

template <typename T, size_t N>
void test_heap_storage_move()
{
    THeapStorage<T> s1(N);
    s1.resize(N); // constructs N
    for (size_t i = 0; i < N; i++)
        s1.data[i] = (int)i;

    THeapStorage<T> s2(std::move(s1));
    CHECK(s2.cap == N);
    CHECK(s2.size == N);
    for (size_t i = 0; i < N; i++)
        CHECK(s2.data[i] == (int)i);

    THeapStorage<T> s3;
    s3 = std::move(s2);
    CHECK(s3.cap == N);
    CHECK(s3.size == N);
    for (size_t i = 0; i < N; i++)
        CHECK(s3.data[i] == (int)i);

    // destructs N
}

TEST_CASE("HeapStorage move")
{
    Foo::reset();

    constexpr size_t N = 4;
    test_heap_storage_move<Foo, N>();
    test_heap_storage_move<int, N>();
    test_heap_storage_move<std::optional<int>, N>();

    const MemoryProfile& profile = get_memory_profile(MEMORY_USAGE_MISC);
    CHECK(profile.current == 0);
    CHECK(Foo::sCtor == N);
    CHECK(Foo::sDtor == N);
    CHECK(Foo::sCopyAssign == 0);
    CHECK(Foo::sCopyCtor == 0);

    // move applies to the container, not the elements
    CHECK(Foo::sMoveAssign == 0);
    CHECK(Foo::sMoveCtor == 0);
}