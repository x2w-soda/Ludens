#pragma once

#include <cstddef>
#include <cstdint>

namespace LD {

enum AssetType
{
    ASSET_TYPE_MESH,
    ASSET_TYPE_TEXTURE_2D,
    ASSET_TYPE_LUA_SCRIPT,
    ASSET_TYPE_ENUM_COUNT,
};

/// @brief asset unique identifier
typedef uint32_t AUID;

size_t get_asset_byte_size(AssetType type);

/// @brief Get static C string for asset type
const char* get_asset_type_cstr(AssetType type);

} // namespace LD