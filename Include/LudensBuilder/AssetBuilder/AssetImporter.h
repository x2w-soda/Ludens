#pragma once

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

    /// @brief Begin a scope for importing.
    void import_batch_begin();

    /// @brief Blocks main thread until all import jobs in batch complete.
    void import_batch_end(Vector<AssetImportResult>& outResults);

    /// @brief Non-blocking update to check for updates.
    ///        Call within batch scope.
    /// @return True if a single result is ready.
    bool import_batch_update(AssetImportResult& outResult);

    /// @brief Non-blocking submission of an import job.
    ///        Call within batch scope.
    void import_batch_asset(Asset dstAsset, const AssetImportInfo* info);

    /// @brief Blocks until a single import process has completed on the calling thread.
    AssetImportResult import_asset_synchronous(Asset dstAsset, const AssetImportInfo* info);
};

} // namespace LD