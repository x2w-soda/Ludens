#pragma once

#include <Extra/doctest/doctest.h>
#include <Ludens/DSA/String.h>
#include <Ludens/DSA/View.h>
#include <Ludens/Header/Types.h>

using namespace LD;

static_assert(LD::IsTrivial<View>);

template <typename T, size_t TLocalSize>
void test_string_ctor()
{
    using Str = TString<T, TLocalSize>;

    Str s;
    CHECK(s.size() == 0);
    CHECK(s.empty());

    const char* cstr = "string";
    s = cstr;
    CHECK(s.size() == 6);

    s.clear();
    CHECK(s.empty());
}

TEST_CASE("String ctor")
{
    constexpr size_t localSize = 12;

    test_string_ctor<char, localSize>();
    test_string_ctor<uint16_t, localSize>();
    test_string_ctor<uint32_t, localSize>();

    const MemoryProfile& profile = get_memory_profile(MEMORY_USAGE_MISC);
    CHECK(profile.current == 0);
}

template <typename T, size_t TLocalSize>
void test_string_copy()
{
    using Str = TString<T, TLocalSize>;

    const char* cstr = "hello, world";

    Str s1(cstr);
    CHECK(s1.size() == 12);

    // copy construct
    Str s2(s1);
    CHECK(s2.size() == s1.size());
    CHECK(s2 == s1);

    // copy assign
    Str s3;
    s3 = s1;
    CHECK(s3.size() == s1.size());
    CHECK(s3 == s2);
}

TEST_CASE("String copy")
{
    constexpr size_t localSize = 12;

    test_string_copy<char, localSize>();
    test_string_copy<uint16_t, localSize>();
    test_string_copy<uint32_t, localSize>();

    const MemoryProfile& profile = get_memory_profile(MEMORY_USAGE_MISC);
    CHECK(profile.current == 0);
}

template <typename T, size_t TLocalSize>
void test_string_move()
{
    using Str = TString<T, TLocalSize>;

    const char* cstr = "string move";
    size_t len = strlen(cstr);

    Str s1(cstr);
    CHECK(s1.size() == len);

    // move construct local string
    Str s2(std::move(s1));
    CHECK(s2.size() == len);
    CHECK(s2 == "string move");

    // move assign local string
    Str s3;
    s3 = std::move(s2);
    CHECK(s3.size() == len);
    CHECK(s3 == "string move");

    s1 = Str(cstr);
    s1.resize(TLocalSize * 2);  // move to heap storage
    s1.resize(len);             // does not revert to local storage
    CHECK(s1 == "string move"); // content should not be truncated

    // move construct heap string
    Str s4(std::move(s1));
    CHECK(s4.size() == len);
    CHECK(s4 == "string move");

    // move assign heap string
    s3 = std::move(s4);
    CHECK(s3.size() == len);
    CHECK(s3 == "string move");
}

TEST_CASE("String move")
{
    constexpr size_t localSize = 12;

    test_string_move<char, localSize>();
    test_string_move<uint16_t, localSize>();
    test_string_move<uint32_t, localSize>();

    const MemoryProfile& profile = get_memory_profile(MEMORY_USAGE_MISC);
    CHECK(profile.current == 0);
}

TEST_CASE("String methods")
{
    {
        String s;

        CHECK(s.capacity() == STRING_DEFAULT_LOCAL_STORAGE);
        CHECK(s.empty());

        s = "foo";
        CHECK(s == "foo");

        s.resize(s.capacity() + 1);
        s.resize(3);
        CHECK(s == "foo");
        CHECK(s.data()[0] == 'f');
        CHECK(s.size() == 3);

        s.clear();
        CHECK(s.size() == 0);
    }

    const MemoryProfile& profile = get_memory_profile(MEMORY_USAGE_MISC);
    CHECK(profile.current == 0);
}

TEST_CASE("String replace")
{
    {
        String s = "foo";

        // nop
        s.replace(0, 0, nullptr, 0);
        CHECK(s == "foo");

        // replacement retains same size
        s.replace(1, 2, "ar", 2);
        CHECK(s == "far");

        // replacement grows string
        s.replace(1, 2, "bar", 3);
        CHECK(s == "fbar");

        // replacement shrinks string
        s.replace(1, 2, nullptr, 0);
        CHECK(s == "fr");
    }

    {
        // replace front
        String s = "abcdef";
        s.replace(0, 2, "XY", 2);
        CHECK(s == "XYcdef");

        // replace end
        s = "abcdef";
        s.replace(4, 2, "XY", 2);
        CHECK(s == "abcdXY");

        // replace middle
        s = "abcdef";
        s.replace(2, 2, "XY", 2);
        CHECK(s == "abXYef");

        // replace with longer string
        s = "abc";
        s.replace(1, 1, "XYZ", 3);
        CHECK(s == "aXYZc");

        // replace with shorter string
        s = "abcdef";
        s.replace(2, 3, "X", 1);
        CHECK(s == "abXf");

        // replace with nothing
        s = "abcdef";
        s.replace(2, 3, nullptr, 0);
        CHECK(s == "abf");

        // append
        s = "abc";
        s.replace(3, 0, "XYZ", 3);
        CHECK(s == "abcXYZ");

        // insert
        s = "abc";
        s.replace(1, 0, "XYZ", 3);
        CHECK(s == "aXYZbc");

        // insert front
        s = ", world";
        s.replace(0, 0, "Hello", 5);
        CHECK(s == "Hello, world");

        // full replace
        s = "abc";
        s.replace(0, 3, "XYZ", 3);
        CHECK(s == "XYZ");

        // nop
        s = "";
        s.replace(0, 0, nullptr, 0);
        CHECK(s == "");
    }
}