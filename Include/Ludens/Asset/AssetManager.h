#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/Asset/AssetRegistry.h>
#include <Ludens/Asset/AssetType/AudioClipAsset.h>
#include <Ludens/Asset/AssetType/LuaScriptAsset.h>
#include <Ludens/Asset/AssetType/MeshAsset.h>
#include <Ludens/Asset/AssetType/Texture2DAsset.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/RenderSystem/RenderSystem.h>
#include <Ludens/System/FileSystem.h>

namespace LD {

struct AssetManagerInfo
{
    FS::Path rootPath;        // project root directory path
    FS::Path assetSchemaPath; // path to asset schema
    bool watchAssets;         // whether to watch asset files in project
};

struct AssetManager : Handle<struct AssetManagerObj>
{
    /// @brief Create asset manager.
    static AssetManager create(const AssetManagerInfo& info);

    /// @brief Destroy asset manager.
    static void destroy(AssetManager manager);

    /// @brief If file watching is enabled, this polls for any changes.
    void update();

    /// @brief Append load jobs to loads all assets in registry.
    void load_all_assets();

    /// @brief Append a load job to current patch.
    void load_asset(AssetType type, AUID auid, const FS::Path& path, const std::string& name);

    /// @brief Begin asset load batch.
    void begin_load_batch();

    /// @brief End asset load batch.
    void end_load_batch();

    /// @brief Get asset ID from name
    AUID get_id_from_name(const char* name, AssetType* outType);

    /// @brief Get asset from ID
    Asset get_asset(AUID auid);

    /// @brief Get asset from ID and expected type, fails upon type mismatch.
    Asset get_asset(AUID auid, AssetType type);

    /// @brief Get asset from name and expected type, fails upon type mismatch.
    Asset get_asset(const char* name, AssetType type);
};

} // namespace LD