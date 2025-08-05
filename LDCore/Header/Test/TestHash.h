#include <Extra/doctest/doctest.h>
#include <Ludens/Header/Hash.h>
#include <unordered_set>

using namespace LD;

TEST_CASE("Hash32")
{
    constexpr const char str[] = "hello, world";
    uint32_t h1 = hash32_FNV_1a(str, strlen(str));
    CHECK(h1 == 0x4d0ea41d);

    // compile time evaluation from bytes
    constexpr uint32_t h2 = hash32_FNV_1a_const(str, sizeof(str) - 1);
    CHECK(h2 == 0x4d0ea41d);

    // compile time evaluation from cstr literal
    constexpr uint32_t h3 = hash32_FNV_1a_const_cstr(str);
    CHECK(h3 == 0x4d0ea41d);

    // Hash32 initialization
    Hash32 h{};
    CHECK(h == Hash32((uint32_t)0));

    // Hash32 constexpr ctor
    constexpr Hash32 h4(str);
    CHECK(h4 == Hash32(0x4d0ea41d));

    // Hash32 runtime evaluation
    Hash32 h5(str, strlen(str));
    CHECK(h5 == Hash32(0x4d0ea41d));
    CHECK(h4 == h5);

    // Hash32 STL support
    std::unordered_set<Hash32> set;
    set.insert(h4);
    CHECK(set.contains(h5));
}

TEST_CASE("Hash64")
{
    constexpr const char str[] = "hello, world";
    uint64_t h1 = hash64_FNV_1a(str, strlen(str));
    CHECK(h1 == 0x17a1a4f267be633d);

    // compile time evaluation from bytes
    constexpr uint64_t h2 = hash64_FNV_1a_const(str, sizeof(str) - 1);
    CHECK(h2 == 0x17a1a4f267be633d);

    // compile time evaluation from cstr literal
    constexpr uint64_t h3 = hash64_FNV_1a_const_cstr(str);
    CHECK(h3 == 0x17a1a4f267be633d);

    // Hash64 initialization
    CHECK(Hash64{} == Hash64((uint64_t)0));

    // Hash64 constexpr ctor
    constexpr Hash64 h4(str);
    CHECK(h4 == Hash64(0x17a1a4f267be633d));

    // Hash64 runtime evaluation
    Hash64 h5(str, strlen(str));
    CHECK(h5 == Hash64(0x17a1a4f267be633d));
    CHECK(h4 == h5);

    // Hash64 STL support
    std::unordered_set<Hash64> set;
    set.insert(h4);
    CHECK(set.contains(h5));
}