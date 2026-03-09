#pragma once

#include <Ludens/DSA/HashSet.h>

#include <cstdint>
#include <format>

#define SUID_IDENTITY_BITS 24
#define SUID_IDENTITY_MASK 0xFFFFFF

namespace LD {

enum SerialType
{
    SERIAL_TYPE_NONE = 0,
    SERIAL_TYPE_ASSET,
    SERIAL_TYPE_COMPONENT,
    SERIAL_TYPE_SCENE,
    SERIAL_TYPE_ENUM_COUNT,
};

/// @brief Serial unique ID in u32 space, zero is invalid ID.
///        This is intended for persistent IDs to be stored on disk.
class SUID
{
public:
    SUID() = default;
    SUID(uint32_t id) : mID(id) {}
    SUID(SerialType type, uint32_t identity)
    {
        mID = (static_cast<uint32_t>(type) << SUID_IDENTITY_BITS) | (identity & SUID_IDENTITY_MASK);
    }

    inline operator bool() const noexcept
    {
        return mID != 0;
    }

    inline operator uint32_t() const noexcept
    {
        return mID;
    }

    inline bool operator==(const SUID& other) const noexcept
    {
        return mID == other.mID;
    }

    inline bool operator!=(const SUID& other) const noexcept
    {
        return mID != other.mID;
    }

    inline SerialType type() const
    {
        return static_cast<SerialType>(mID >> SUID_IDENTITY_BITS);
    }

private:
    uint32_t mID;
};

/// @brief Non-thread safe static SUID distributor.
class SUIDRegistry
{
public:
    /// @brief Acquire next valid SUID.
    static SUID get_suid(SerialType type);

    /// @brief Try acquire a SUID, this is intended for deserialization code paths where the ID is known.
    ///        Fails if the ID is invalid or already used.
    static bool try_get_suid(SUID id);

    /// @brief Release a registered SUID.
    static void free_suid(SUID id);

private:
    static HashSet<SUID> sRegistered;
    static uint32_t sCounter[SERIAL_TYPE_ENUM_COUNT];
};

} // namespace LD

template <>
struct std::formatter<LD::SUID> : std::formatter<uint32_t>
{
    auto format(const LD::SUID& id, std::format_context& ctx) const
    {
        return std::formatter<uint32_t>::format(static_cast<uint32_t>(id), ctx);
    }
};

template <>
struct std::hash<LD::SUID>
{
    size_t operator()(const LD::SUID& id) const noexcept
    {
        return std::hash<uint32_t>{}(static_cast<uint32_t>(id));
    }
};
