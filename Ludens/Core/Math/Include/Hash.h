#pragma once

#include <cstddef>

// A collection of some better knowh hashes online.
// Note that these hashes are not ideal for cryptographic use.
// - djb2 and other string hashes:
//   http://www.cse.yorku.ca/~oz/hash.html
// - hash combine from the boost library:
//   https://www.boost.org/doc/libs/1_85_0/libs/container_hash/doc/html/hash.html#notes_hash_combine

namespace LD
{

template <typename TChar>
struct DJB2
{
    size_t operator()(const TChar* str, size_t len)
    {
        size_t hash = 5381;

        for (size_t i = 0; i < len; i++)
        {
            hash = ((hash << 5) + hash) + str[i]; // hash * 33 + str[i]
        }

        return hash;
    }
};

template <typename... TArgs>
inline void HashCombine(size_t& seed, size_t hash, TArgs... args)
{
    seed ^= hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    (HashCombine(seed, args), ...);
}

} // namespace LD