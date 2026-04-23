#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/System/FileSystem.h>

#include <string>

namespace LD {

/// @brief Base asset import info. An Asset is associated with a Project via import.
struct AssetImportInfo
{
    const AssetType type; /// desired import asset type
    std::string dstPath;  /// requested asset URI path

    AssetImportInfo() = delete;
    AssetImportInfo(AssetType type)
        : type(type) {}

    /// @brief Set source file paths
    /// @return Whether the configuration is accepted.
    bool set_src_files(size_t pathCount, const FS::Path* paths);
};

enum AssetCreateType
{
    ASSET_CREATE_TYPE_LUA_SCRIPT,
    ASSET_CREATE_TYPE_ENUM_COUNT,
};

/// @brief Base asset creation info. The engine can create some assets ready for import.
struct AssetCreateInfo
{
    const AssetCreateType type;
    const AssetType assetType;

    AssetCreateInfo() = delete;
    AssetCreateInfo(AssetCreateType type, AssetType assetType)
        : type(type), assetType(assetType) {}
};

} // namespace LD