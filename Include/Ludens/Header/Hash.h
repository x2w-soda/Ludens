#pragma once

#include <cstdint>
#include <functional>
#include <string>
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

/// @brief 32-bit hash value
class Hash32
{
public:
    Hash32() : mHash(0) {}

    Hash32(const std::string& str)
        : mHash(hash32_FNV_1a(str.data(), str.size()))
    {
    }

    Hash32(std::string&& str)
        : mHash(hash32_FNV_1a(str.data(), str.size()))
    {
    }

    Hash32(const char* cstr)
        : mHash(hash32_FNV_1a(cstr, strlen(cstr)))
    {
    }

    inline operator uint32_t() const { return mHash; }
    inline bool operator==(const Hash32& other) const { return mHash == other.mHash; }
    inline bool operator!=(const Hash32& other) const { return mHash != other.mHash; }

private:
    uint32_t mHash;
};

} // namespace LD

namespace std {

template <>
struct hash<LD::Hash32>
{
    size_t operator()(const LD::Hash32& s) const
    {
        return (size_t)s;
    }
};

} // namespace std