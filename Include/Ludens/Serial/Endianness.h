#pragma once

#include <Ludens/Header/Types.h>
#include <cstdint>

namespace LD {

inline void le_u16_to_bytes(uint16_t u16, byte bytes[2])
{
    bytes[0] = u16 & 0xFF;
    bytes[1] = (u16 >> 8) & 0xFF;
}

inline void le_i16_to_bytes(int16_t i16, byte bytes[2])
{
    bytes[0] = i16 & 0xFF;
    bytes[1] = (i16 >> 8) & 0xFF;
}

inline void le_u32_to_bytes(uint32_t u32, byte bytes[4])
{
    bytes[0] = u32 & 0xFF;
    bytes[1] = (u32 >> 8) & 0xFF;
    bytes[2] = (u32 >> 16) & 0xFF;
    bytes[3] = (u32 >> 24) & 0xFF;
}

inline void le_i32_to_bytes(int32_t i32, byte bytes[4])
{
    bytes[0] = i32 & 0xFF;
    bytes[1] = (i32 >> 8) & 0xFF;
    bytes[2] = (i32 >> 16) & 0xFF;
    bytes[3] = (i32 >> 24) & 0xFF;
}

inline void le_u64_to_bytes(uint64_t u64, byte bytes[8])
{
    bytes[0] = u64 & 0xFF;
    bytes[1] = (u64 >> 8) & 0xFF;
    bytes[2] = (u64 >> 16) & 0xFF;
    bytes[3] = (u64 >> 24) & 0xFF;
    bytes[4] = (u64 >> 32) & 0xFF;
    bytes[5] = (u64 >> 40) & 0xFF;
    bytes[6] = (u64 >> 48) & 0xFF;
    bytes[7] = (u64 >> 56) & 0xFF;
}

inline void le_i64_to_bytes(int64_t i64, byte bytes[8])
{
    bytes[0] = i64 & 0xFF;
    bytes[1] = (i64 >> 8) & 0xFF;
    bytes[2] = (i64 >> 16) & 0xFF;
    bytes[3] = (i64 >> 24) & 0xFF;
    bytes[4] = (i64 >> 32) & 0xFF;
    bytes[5] = (i64 >> 40) & 0xFF;
    bytes[6] = (i64 >> 48) & 0xFF;
    bytes[7] = (i64 >> 56) & 0xFF;
}

inline void le_bytes_to_u16(const byte bytes[2], uint16_t& u16)
{
    u16 = 0;
    u16 |= bytes[0];
    u16 |= bytes[1] << 8;
}

inline void le_bytes_to_i16(const byte bytes[2], int16_t& i16)
{
    i16 = 0;
    i16 |= bytes[0];
    i16 |= bytes[1] << 8;
}

inline void le_bytes_to_u32(const byte bytes[4], uint32_t& u32)
{
    u32 = 0;
    u32 |= bytes[0];
    u32 |= bytes[1] << 8;
    u32 |= bytes[2] << 16;
    u32 |= bytes[3] << 24;
}

inline void le_bytes_to_i32(const byte bytes[4], int32_t& i32)
{
    i32 = 0;
    i32 |= bytes[0];
    i32 |= bytes[1] << 8;
    i32 |= bytes[2] << 16;
    i32 |= bytes[3] << 24;
}

inline void le_bytes_to_u64(const byte bytes[8], uint64_t& u64)
{
    u64 = 0;
    u64 |= (uint64_t)bytes[0];
    u64 |= (uint64_t)bytes[1] << 8;
    u64 |= (uint64_t)bytes[2] << 16;
    u64 |= (uint64_t)bytes[3] << 24;
    u64 |= (uint64_t)bytes[4] << 32;
    u64 |= (uint64_t)bytes[5] << 40;
    u64 |= (uint64_t)bytes[6] << 48;
    u64 |= (uint64_t)bytes[7] << 56;
}

inline void le_bytes_to_i64(const byte bytes[8], int64_t& i64)
{
    i64 = 0;
    i64 |= (uint64_t)bytes[0];
    i64 |= (uint64_t)bytes[1] << 8;
    i64 |= (uint64_t)bytes[2] << 16;
    i64 |= (uint64_t)bytes[3] << 24;
    i64 |= (uint64_t)bytes[4] << 32;
    i64 |= (uint64_t)bytes[5] << 40;
    i64 |= (uint64_t)bytes[6] << 48;
    i64 |= (uint64_t)bytes[7] << 56;
}

} // namespace LD