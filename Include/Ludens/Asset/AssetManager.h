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
    FS::Path rootPath;      // project root directory path
    AssetRegistry registry; // referenced asset registry, out-lives the AssetManager
    bool watchAssets;       // whether to watch asset files in project
};

/// @brief Asset manager singleton, main thread only.
struct AssetManager : Handle<struct AssetManagerObj>
{
    /// @brief Create asset manager.
    static AssetManager create(const AssetManagerInfo& info);

    /// @brief Destroy asset manager.
    static void destroy();

    /// @brief Get asset manager singleton.
    static AssetManager get();

    AssetRegistry get_asset_registry();

    AssetRegistry swap_asset_registry(AssetRegistry registry, const FS::Path& rootPath);

    /// @brief If file watching is enabled, this polls for any changes.
    void update();

    /// @brief Append load jobs to loads all assets in registry.
    void load_all_assets();

    /// @brief Append a load job to current patch.
    void load_asset(AssetID id);

    /// @brief Begin asset load batch.
    void begin_load_batch();

    /// @brief End asset load batch.
    bool end_load_batch(Vector<std::string>& outErrors);

    /// @brief Check if there are any load jobs in progress. Does not block.
    bool has_load_job();

    /// @brief Get asset ID from name
    AssetID get_id_from_name(const char* name, AssetType* outType);

    /// @brief Get asset from ID
    Asset get_asset(AssetID id);

    /// @brief Get asset from ID and expected type, fails upon type mismatch.
    Asset get_asset(AssetID id, AssetType type);

    /// @brief Get asset from name and expected type, fails upon type mismatch.
    Asset get_asset(const char* name, AssetType type);

    /// @brief Allocate an empty asset that needs to be resolved later.
    ///        Intended for AssetImporter to perform "import + load" simultaneously.
    Asset reserve_asset(AssetType type);

    /// @brief Resolve the asset against the current AssetRegistry.
    ///        Intended for AssetImporter to add an Asset to the project.
    AssetEntry resolve_asset(SUIDRegistry idReg, Asset asset, const std::string& uri);
};

} // namespace LD