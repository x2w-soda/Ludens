#pragma once

#include <Ludens/DSA/Vector.h>

#include <cstdint>
#include <format>

namespace LD {

class ID
{
public:
    ID() = default;
    ID(uint64_t id) : mID(id) {}

    /// @brief ID is invalid if and only if both index and generation is zero.
    inline operator bool() const noexcept
    {
        return mID != 0;
    }

    inline operator uint64_t() const noexcept
    {
        return mID;
    }

    inline bool operator==(const ID& other) const noexcept
    {
        return mID == other.mID;
    }

    inline bool operator!=(const ID& other) const noexcept
    {
        return mID != other.mID;
    }

    inline uint32_t index() const
    {
        return mID & 0xFFFFFF;
    }

    inline uint64_t generation() const
    {
        return mID >> 24;
    }

private:
    uint64_t mID;
};

/// @brief Non thread-safe ID registry. Biased to reuse lower indices.
///        A maximum of 2^24 IDs can be used simultaneously.
class IDRegistry
{
public:
    ID create()
    {
        uint32_t index;

        if (!mFreeIndexList.empty())
        {
            // This gives a bias towards lower unused index,
            // but we have 40-bits of generation headrooom.
            index = mFreeIndexList.back();
            mFreeIndexList.pop_back();
        }
        else
        {
            index = static_cast<uint32_t>(mGenerations.size());
            if (index >= 0xFFFFFF)
                return 0;

            mGenerations.push_back(1);
        }

        uint64_t gen = mGenerations[index];
        return (gen << 24) | (index & 0xFFFFFF);
    }

    void destroy(ID id)
    {
        if (!id)
            return;

        uint32_t index = id.index();
        uint64_t gen = id.generation();

        if (index < mGenerations.size() && mGenerations[index] == gen)
        {
            mGenerations[index]++;
            mFreeIndexList.push_back(index);
        }
    }

private:
    Vector<uint64_t> mGenerations;
    Vector<uint32_t> mFreeIndexList;
};

} // namespace LD

template <>
struct std::formatter<LD::ID> : std::formatter<uint64_t>
{
    auto format(const LD::ID& id, std::format_context& ctx) const
    {
        return std::formatter<uint64_t>::format(static_cast<uint64_t>(id), ctx);
    }
};

template <>
struct std::hash<LD::ID>
{
    size_t operator()(const LD::ID& id) const noexcept
    {
        return std::hash<uint64_t>{}(static_cast<uint64_t>(id));
    }
};

