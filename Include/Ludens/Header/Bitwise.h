#pragma once

#include <cstdint>

#define LD_BIT(B) (1ul << (B))

/// @brief round up to the next highest power of 2
///        from Bit Twiddling Hacks https://graphics.stanford.edu/%7Eseander/bithacks.html#RoundUpPowerOf2
/// @param v 32-bit unsigned value to query
/// @return 32-bit power of two value greater or equal to \p v
/// @warning has an edge case where 0 is returned if \p v is 0.
inline uint32_t next_power_of_two(uint32_t v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}