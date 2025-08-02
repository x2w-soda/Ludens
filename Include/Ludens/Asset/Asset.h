#pragma once

#include <cstddef>
#include <cstdint>

namespace LD {

enum AssetType
{
    ASSET_TYPE_MESH,
    ASSET_TYPE_TEXTURE_2D,
    ASSET_TYPE_ENUM_COUNT,
};

/// @brief asset unique identifier
typedef uint32_t AUID;

size_t get_asset_byte_size(AssetType type);

} // namespace LD