#pragma once

#include <Ludens/Header/Handle.h>
#include <cstddef>
#include <cstdint>

namespace LD {

enum AssetType
{
    ASSET_TYPE_AUDIO_CLIP,
    ASSET_TYPE_FONT,
    ASSET_TYPE_MESH,
    ASSET_TYPE_TEXTURE_2D,
    ASSET_TYPE_LUA_SCRIPT,
    ASSET_TYPE_ENUM_COUNT,
};

/// @brief Asset unique identifier.
typedef uint32_t AUID;

/// @brief Get byte size of an asset type.
size_t get_asset_byte_size(AssetType type);

/// @brief Get static C string for asset type.
const char* get_asset_type_cstr(AssetType type);

struct AssetHandle : Handle<struct AssetObj>
{
    AssetHandle() = default;
    AssetHandle(AssetObj* obj)
        : Handle<AssetObj>(obj) {}

    /// @brief Get asset type.
    AssetType get_type();

    /// @brief Get asset name.
    const char* get_name();

    /// @brief Get asset identifier.
    AUID get_auid();
};

} // namespace LD