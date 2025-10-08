#include <Extra/doctest/doctest.h>
#include <Ludens/Serial/Endianness.h>
#include <cstdint>
#include <limits>

using namespace LD;

TEST_CASE("2-Byte Little Endian")
{
    uint16_t u16 = 0xCAFE;
    byte bytes[2];

    le_u16_to_bytes(u16, bytes);
    CHECK(bytes[0] == 0xFE);
    CHECK(bytes[1] == 0xCA);

    le_bytes_to_u16(bytes, u16);
    CHECK(u16 == 0xCAFE);

    int16_t i16 = std::numeric_limits<int16_t>::min();
    le_i16_to_bytes(i16, bytes);
    CHECK(bytes[0] == 0);
    CHECK(bytes[1] == 0b10000000);

    le_bytes_to_i16(bytes, i16);
    CHECK(i16 == std::numeric_limits<int16_t>::min());
}

TEST_CASE("4-Byte Little Endian")
{
    uint32_t u32 = 0xCAFEBABE;
    byte bytes[4];

    le_u32_to_bytes(u32, bytes);
    CHECK(bytes[0] == 0xBE);
    CHECK(bytes[1] == 0xBA);
    CHECK(bytes[2] == 0xFE);
    CHECK(bytes[3] == 0xCA);

    le_bytes_to_u32(bytes, u32);
    CHECK(u32 == 0xCAFEBABE);

    int32_t i32 = std::numeric_limits<int32_t>::min();
    le_i32_to_bytes(i32, bytes);
    CHECK(bytes[0] == 0);
    CHECK(bytes[1] == 0);
    CHECK(bytes[2] == 0);
    CHECK(bytes[3] == 0b10000000);

    le_bytes_to_i32(bytes, i32);
    CHECK(i32 == std::numeric_limits<int32_t>::min());
}

TEST_CASE("8-Byte Little Endian")
{
    uint64_t u64 = 0xCAFEBABEDEADBEEF;
    byte bytes[8];

    le_u64_to_bytes(u64, bytes);
    CHECK(bytes[0] == 0xEF);
    CHECK(bytes[1] == 0xBE);
    CHECK(bytes[2] == 0xAD);
    CHECK(bytes[3] == 0xDE);
    CHECK(bytes[4] == 0xBE);
    CHECK(bytes[5] == 0xBA);
    CHECK(bytes[6] == 0xFE);
    CHECK(bytes[7] == 0xCA);

    le_bytes_to_u64(bytes, u64);
    CHECK(u64 == 0xCAFEBABEDEADBEEF);

    int64_t i64 = std::numeric_limits<int64_t>::min();
    le_i64_to_bytes(i64, bytes);
    CHECK(bytes[0] == 0);
    CHECK(bytes[1] == 0);
    CHECK(bytes[2] == 0);
    CHECK(bytes[3] == 0);
    CHECK(bytes[4] == 0);
    CHECK(bytes[5] == 0);
    CHECK(bytes[6] == 0);
    CHECK(bytes[7] == 0b10000000);

    le_bytes_to_i64(bytes, i64);
    CHECK(i64 == std::numeric_limits<int64_t>::min());
}