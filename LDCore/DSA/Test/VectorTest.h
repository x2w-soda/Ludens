#pragma once

#include "DSATest.h"
#include <Extra/doctest/doctest.h>
#include <Ludens/DSA/Vector.h>

using namespace LD;

#if 0
template <typename T, size_t N>
void test_svector()
{
    SVector<T, N> v1;
    CHECK(v1.size() == 0);

    v1.resize(N);
    CHECK(v1.size() == N);

    for (size_t i = 0; i < v1.size(); i++)
        v1[i] = (int)i;

    v1.resize(2 * N);
    CHECK(v1.size() == 2 * N);

    // realloc behavior
    for (int i = 0; i < N; i++)
        CHECK(v1[i] == i);

    v1.clear();
    CHECK(v1.size() == 0);
    CHECK(v1.empty());

    SVector<T, N> v2(2 * N, T{12345});
    CHECK(v2.size() == 2 * N);

    for (size_t i = 0; i < v2.size(); i++)
        CHECK(v2[i] == 12345);

    SVector<Foo, N> v3(2 * N);
    CHECK(v3.size() == 2 * N);

    SVector<std::string, 2> v4(1);
    std::string lvalue("def");
    v4[0] = "abc";
    v4.push_back(lvalue);
    v4.push_back(std::string("ghi"));
    v4.emplace_back("jkl");

    CHECK(v4.size() == 4);
    CHECK(v4[0] == "abc");
    CHECK(v4[1] == "def");
    CHECK(v4[2] == "ghi");
    CHECK(v4[3] == "jkl");
}

TEST_CASE("SVector")
{
    constexpr size_t N = 4;

    test_svector<int, N>();
    test_svector<Foo, N>();

    const MemoryProfile& profile = get_memory_profile(MEMORY_USAGE_MISC);
    CHECK(profile.current == 0);
}

template <typename T, size_t N>
void test_svector_copy()
{
    SVector<T, N> v1;
    SVector<T, N> v2;

    v1.resize(N);
    for (size_t i = 0; i < v1.size(); i++)
        v1[i] = (int)i;

    CHECK(v2.empty());

    // copy assign
    v2 = v1;
    v1.clear();
    CHECK(v1.empty());
    CHECK(v2.size() == N);

    for (size_t i = 0; i < N; i++)
        CHECK(v2[i] == (int)i);

    // copy construct
    SVector<T, N> v3(v2);
    v2.clear();
    CHECK(v2.empty());
    CHECK(v3.size() == N);

    for (size_t i = 0; i < N; i++)
        CHECK(v3[i] == (int)i);
}

TEST_CASE("SVector copy")
{
    constexpr size_t N = 4;

    test_svector_copy<int, N>();
    test_svector_copy<Foo, N>();

    const MemoryProfile& profile = get_memory_profile(MEMORY_USAGE_MISC);
    CHECK(profile.current == 0);
}

template <typename T, size_t N>
void test_svector_move()
{
    SVector<T, N> v1(N);
    SVector<T, N> v2;

    CHECK(v1.size() == N);
    CHECK(v2.empty());

    for (size_t i = 0; i < v1.size(); i++)
        v1[i] = (int)i;

    // move assign local storage
    v2 = std::move(v1);
    CHECK(v2.size() == N);

    for (size_t i = 0; i < N; i++)
        CHECK(v2[i] == (int)i);

    // move construct local storage
    SVector<T, N> v3(std::move(v2));
    CHECK(v3.size() == N);

    for (size_t i = 0; i < N; i++)
        CHECK(v3[i] == (int)i);

    SVector<T, N> v4(2 * N); // using heap storage
    CHECK(v4.size() == 2 * N);

    for (size_t i = 0; i < v4.size(); i++)
        v4[i] = (int)i;

    // move assign heap storage
    SVector<T, N> v5;
    v5 = std::move(v4);

    CHECK(v5.size() == 2 * N);
    for (size_t i = 0; i < v5.size(); i++)
        CHECK(v5[i] == (int)i);

    // move construct heap storage
    SVector<T, N> v6(std::move(v5));

    CHECK(v6.size() == 2 * N);
    for (size_t i = 0; i < v6.size(); i++)
        CHECK(v6[i] == (int)i);
}

TEST_CASE("SVector move")
{
    constexpr size_t N = 4;

    test_svector_move<int, N>();
    test_svector_move<Foo, N>();

    const MemoryProfile& profile = get_memory_profile(MEMORY_USAGE_MISC);
    CHECK(profile.current == 0);
}
#endif