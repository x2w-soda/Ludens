#pragma once

#include <Ludens/Asset/Asset.h>
#include <LudensBuilder/AssetBuilder/AssetImportInfoStorage.h>
#include <LudensBuilder/AssetBuilder/AssetImporter.h>

#include <atomic>

namespace LD {

/// @brief Worker thread user data for asset import.
///        Needs address stability.
struct AssetImportJob
{
    Asset asset;                           // destination asset handle to populate
    AssetImportInfoStorage info;           // source import info
    AssetImportStatus status;              // resulting status
    std::atomic_bool hasCompleted = false; // polled by main thread

    inline bool has_completed()
    {
        return hasCompleted.load(std::memory_order_acquire);
    }

    /// @brief Check for predicate, update status upon failure.
    inline bool require(bool pred, const char* str)
    {
        if (!pred)
        {
            status.str = str;
            status.type = ASSET_IMPORT_ERROR;
            return false;
        }

        return true;
    }

    /// @brief Try reading src path to vector, updates status upon failure.
    inline bool read_src_path_to_vector(const FS::Path& path, Vector<byte>& v)
    {
        if (!FS::read_file_to_vector(path, v, status.str))
        {
            status.type = ASSET_IMPORT_ERROR_SRC_PATH;
            return false;
        }

        return true;
    }

    /// @brief Try write to destination file, updates status upon failure.
    inline void write_to_dst_path(const View& view)
    {
        FS::Path dstPath = static_cast<AssetImportInfo*>(&info.as.blob)->dstPath;

        if (!FS::write_file(dstPath, view, status.str))
            status.type = ASSET_IMPORT_ERROR_DST_PATH;
    }

    /// @brief Copy transient info into job local storage.
    void copy_info(const AssetImportInfo* info);

    /// @brief Submit to job system, after which main thread should not
    ///        access member fields until job completion.
    void submit();

    /// @brief Direct synchronous execution on calling thread.
    ///        Does not leverage the job system.
    void execute_synchronous();
};

} // namespace LD