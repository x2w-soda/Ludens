#pragma once

#include <Ludens/Asset/AssetRegistry.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/Header/Status.h>
#include <LudensBuilder/AssetBuilder/AssetImportInfo.h>

namespace LD {

enum AssetImportStatusType
{
    ASSET_IMPORT_SUCCESS,
    ASSET_IMPORT_ERROR,
    ASSET_IMPORT_ERROR_SRC_PATH,
    ASSET_IMPORT_ERROR_DST_PATH,
    ASSET_IMPORT_ERROR_DST_URI,
};

struct AssetImportStatus : TStatus<AssetImportStatusType>
{
    inline operator bool() const noexcept
    {
        return type == ASSET_IMPORT_SUCCESS;
    }
};

struct AssetImportResult
{
    AssetImportStatus status; // status code
    Asset dstAsset;           // destination asset handle
};

/// @brief Asynchronous Asset importer.
///        Main thread only.
struct AssetImporter : Handle<struct AssetImporterObj>
{
    static AssetImporter create();
    static void destroy(AssetImporter importer);

    /// @brief Allocate import info for asset type.
    AssetImportInfo* allocate_import_info(AssetType type);

    /// @brief Free an import info.
    void free_import_info(AssetImportInfo* asset);

    /// @brief Begin a scope for importing.
    void import_batch_begin();

    /// @brief End the batch scope.
    void import_batch_end();

    /// @brief Non-blocking submission of an import job.
    ///        Call within batch scope.
    /// @warn Frees the import info.
    void import_batch_asset(AssetImportInfo* info);

    /// @brief Non-blocking poll to check for import results.
    ///        Call outside of batch scope.
    /// @return True if a single result is ready.
    bool import_asset_async(AssetImportResult& outResult);

    /// @brief Blocks until a single import process has completed on the calling thread.
    /// @warn Frees the import info.
    AssetImportResult import_asset_synchronous(AssetImportInfo* info);
};

} // namespace LD