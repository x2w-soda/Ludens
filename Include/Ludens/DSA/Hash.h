#pragma once

#include <cstdint>
#include <utility>

namespace LD {

template <typename T>
inline void hash_combine(std::size_t& seed, const T& val)
{
    seed ^= std::hash<T>{}(val) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

/// @brief 32-bit FNV-1a hash function
/// @warning non-cryptographic
inline uint32_t hash32_FNV_1a(const char* bytes, int length)
{
    uint32_t hash = 2166136261;

    for (int i = 0; i < length; i++)
    {
        hash ^= bytes[i];
        hash *= 16777619;
    }

    return hash;
}

} // namespace LD