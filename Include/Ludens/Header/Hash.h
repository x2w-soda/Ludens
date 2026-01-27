#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <type_traits>
#include <utility>

#define HASH32_FNV_1A_PRIME ((uint32_t)16777619)
#define HASH32_FNV_1A_IV ((uint32_t)2166136261)
#define HASH64_FNV_1A_PRIME ((uint64_t)0x100000001b3)
#define HASH64_FNV_1A_IV ((uint64_t)0xcbf29ce484222325)

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
    uint32_t hash = HASH32_FNV_1A_IV;

    for (int i = 0; i < length; i++)
    {
        hash ^= bytes[i];
        hash *= HASH32_FNV_1A_PRIME;
    }

    return hash;
}

/// @brief compile time 32-bit FNV-1a hash function
/// @warning non-cryptographic
inline constexpr uint32_t hash32_FNV_1a_const(const char* bytes, int length, const uint32_t value = HASH32_FNV_1A_IV)
{
    return (length > 0) ? hash32_FNV_1a_const(bytes + 1, length - 1, (value ^ uint32_t(*bytes)) * HASH32_FNV_1A_PRIME) : value;
}

/// @brief compile time 32-bit FNV-1a hash function from string literal
/// @warning non-cryptographic
inline constexpr uint32_t hash32_FNV_1a_const_cstr(const char* cstr, const uint32_t value = HASH32_FNV_1A_IV)
{
    return (*cstr) ? hash32_FNV_1a_const_cstr(cstr + 1, (value ^ uint32_t(*cstr)) * HASH32_FNV_1A_PRIME) : value;
}

/// @brief 64-bit FNV-1a hash function
/// @warning non-cryptographic
inline uint64_t hash64_FNV_1a(const char* bytes, int length)
{
    uint64_t hash = HASH64_FNV_1A_IV;

    for (int i = 0; i < length; i++)
    {
        hash ^= bytes[i];
        hash *= HASH64_FNV_1A_PRIME;
    }

    return hash;
}

/// @brief compile time 64-bit FNV-1a hash function
/// @warning non-cryptographic
inline constexpr uint64_t hash64_FNV_1a_const(const char* bytes, int length, const uint64_t value = HASH64_FNV_1A_IV)
{
    return (length > 0) ? hash64_FNV_1a_const(bytes + 1, length - 1, (value ^ uint64_t(*bytes)) * HASH64_FNV_1A_PRIME) : value;
}

/// @brief compile time 64-bit FNV-1a hash function from string literal
/// @warning non-cryptographic
inline constexpr uint64_t hash64_FNV_1a_const_cstr(const char* cstr, const uint64_t value = HASH64_FNV_1A_IV)
{
    return (*cstr) ? hash64_FNV_1a_const_cstr(cstr + 1, (value ^ uint64_t(*cstr)) * HASH64_FNV_1A_PRIME) : value;
}

/// @brief 32-bit hash value
class Hash32
{
public:
    Hash32() = default;

    Hash32(uint32_t hash)
        : mHash(hash)
    {
    }

    Hash32(const std::string& str)
        : mHash(hash32_FNV_1a(str.data(), (int)str.size()))
    {
    }

    Hash32(std::string&& str)
        : mHash(hash32_FNV_1a(str.data(), (int)str.size()))
    {
    }

    Hash32(const char* bytes, int length)
        : mHash(hash32_FNV_1a(bytes, length))
    {
    }

    constexpr Hash32(const char* cstr)
        : mHash(hash32_FNV_1a_const_cstr(cstr))
    {
    }

    Hash32& operator=(uint32_t hash)
    {
        mHash = hash;
        return *this;
    }

    inline operator uint32_t() const { return mHash; }
    inline bool operator==(const Hash32& other) const { return mHash == other.mHash; }
    inline bool operator!=(const Hash32& other) const { return mHash != other.mHash; }

private:
    uint32_t mHash;
};

static_assert(std::is_trivial<Hash32>::value);

/// @brief 64-bit hash value
class Hash64
{
public:
    Hash64() = default;

    Hash64(uint64_t hash)
        : mHash(hash)
    {
    }

    Hash64(const std::string& str)
        : mHash(hash64_FNV_1a(str.data(), (int)str.size()))
    {
    }

    Hash64(std::string&& str)
        : mHash(hash64_FNV_1a(str.data(), (int)str.size()))
    {
    }

    Hash64(const char* bytes, int length)
        : mHash(hash64_FNV_1a(bytes, length))
    {
    }

    constexpr Hash64(const char* cstr)
        : mHash(hash64_FNV_1a_const_cstr(cstr))
    {
    }

    Hash64& operator=(uint64_t hash)
    {
        mHash = hash;
        return *this;
    }

    inline operator uint64_t() const { return mHash; }
    inline bool operator==(const Hash64& other) const { return mHash == other.mHash; }
    inline bool operator!=(const Hash64& other) const { return mHash != other.mHash; }

private:
    uint64_t mHash;
};

static_assert(std::is_trivial<Hash64>::value);

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

template <>
struct hash<LD::Hash64>
{
    size_t operator()(const LD::Hash64& s) const
    {
        return (size_t)s;
    }
};

} // namespace std