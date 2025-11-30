#pragma once

#include <Ludens/Header/Handle.h>
#include <cstddef>
#include <cstdint>

namespace LD {

enum AssetType
{
    ASSET_TYPE_AUDIO_CLIP,
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

/// @brief Base members of asset object implementation.
struct AssetObj
{
    const char* name;
    struct AssetManagerObj* manager;
    AUID auid;
    AssetType type;
};

/// @brief Asset handle, no reference counting.
/// @tparam TAssetObj Derived class of AssetObj
template <typename TAssetObj>
struct AssetHandle : public Handle<TAssetObj>
{
    AssetHandle() = default;
    AssetHandle(AssetObj* obj)
        : Handle<TAssetObj>((TAssetObj*)obj) {}

    /// @brief Get asset name.
    const char* get_name() { return ((AssetObj*)(this->mObj))->name; }

    /// @brief Get asset identifier.
    AUID get_auid() const { return ((AssetObj*)(this->mObj))->auid; }
};

} // namespace LD