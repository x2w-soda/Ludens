#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/Asset/AssetRegistry.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/Header/Status.h>
#include <Ludens/System/FileSystem.h>

namespace LD {

enum AssetLoadStatusType
{
    ASSET_LOAD_SUCCESS,
    ASSET_LOAD_ERROR,
    ASSET_LOAD_ERROR_FILE_PATH,
};

struct AssetLoadStatus : TStatus<AssetLoadStatusType>
{
    inline operator bool() const noexcept
    {
        return type == ASSET_LOAD_SUCCESS;
    }
};

/// @brief Asset manager environmental context.
struct AssetManagerEnv
{
    FS::Path rootPath = {};          // absolute path to project root directory
    FS::Path storageDir = "storage"; // relative path to storage directory
    AssetRegistry registry = {};     // referenced asset registry

    inline AssetEntry get_entry(SUID id)
    {
        return registry ? registry.get_entry(id) : AssetEntry();
    }

    /// @brief Get aboslute path to asset storage directory
    inline FS::Path get_asset_dir_path(AssetID id)
    {
        return FS::absolute(rootPath / storageDir / FS::Path(id.to_string()));
    }
};

struct AssetManagerInfo
{
    AssetManagerEnv env; // initial environment
    bool watchAssets;    // whether to watch asset files in project
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
    bool end_load_batch(Vector<AssetLoadStatus>& outErrors);

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
    Asset alloc_reserved_asset(SUIDRegistry idReg, AssetType type);
    void free_reserved_asset(SUIDRegistry idReg, Asset reservedAsset);

    /// @brief Resolve the asset against the current AssetRegistry.
    ///        Intended for AssetImporter to add an Asset to the project.
    /// @return A valid entry associated with the Asset upon success.
    /// @note Upon failure, the reserved asset should be freed.
    AssetEntry resolve_asset(SUIDRegistry idReg, Asset reservedAsset, const std::string& uriPath);
};

} // namespace LD