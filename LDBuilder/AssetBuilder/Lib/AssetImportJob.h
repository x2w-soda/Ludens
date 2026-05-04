#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/DSA/HashMap.h>
#include <Ludens/DSA/String.h>
#include <Ludens/Header/Assert.h>
#include <LudensBuilder/AssetBuilder/AssetImporter.h>

#include <atomic>

namespace LD {

/// @brief Worker thread user data for asset import.
///        Needs address stability.
struct AssetImportJob
{
    Asset asset;                           // destination asset handle to populate
    AssetImportInfo* info = nullptr;       // source import info, memory owned by importer
    AssetImportStatus status;              // resulting status
    FS::Path assetDir;                     // asset directory to dump destination files
    std::atomic_bool hasCompleted = false; // polled by main thread
    HashMap<std::string, FS::Path> files;  // destination files written

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
    inline bool read_src_file_to_vector(const FS::Path& path, Vector<byte>& v)
    {
        if (!FS::read_file_to_vector(path, v, status.str))
        {
            status.type = ASSET_IMPORT_ERROR_SRC_FILE;
            return false;
        }

        return true;
    }

    /// @brief Try reading src path to string, updates status upon failure.
    inline bool read_src_file_to_string(const FS::Path& path, std::string& str)
    {
        uint64_t fileSize;
        if (!FS::get_file_size(path, fileSize, status.str) || fileSize == 0)
        {
            status.type = ASSET_IMPORT_ERROR_SRC_FILE;
            return false;
        }

        str.resize(fileSize);

        if (!FS::read_file(path, MutView((byte*)str.data(), str.size()), status.str))
        {
            status.type = ASSET_IMPORT_ERROR_SRC_FILE;
            return false;
        }

        return true;
    }

    /// @brief Try reading src file to mutable view, updates status upon failure.
    inline bool read_src_file(const FS::Path& path, MutView view)
    {
        if (!FS::read_file(path, view, status.str))
        {
            status.type = ASSET_IMPORT_ERROR_SRC_FILE;
            return false;
        }

        return true;
    }

    /// @brief Try write to destination file, updates status upon failure.
    inline bool write_dst_file(const std::string& key, const FS::Path& relPath, View view)
    {
        if (!FS::create_directories(assetDir, status.str))
        {
            status.type = ASSET_IMPORT_ERROR_DST_FILE;
            return false;
        }

        FS::Path dstFilePath = FS::absolute(assetDir / relPath);

        if (!FS::write_file(dstFilePath, view, status.str))
        {
            status.type = ASSET_IMPORT_ERROR_DST_FILE;
            return false;
        }

        LD_ASSERT(!files.contains(key));
        files[key] = relPath;

        return true;
    }

    inline bool write_binary_dst_file(View view)
    {
        return write_dst_file(LD_ASSET_DEFAULT_BINARY_FILE_KEY, LD_ASSET_DEFAULT_BINARY_FILE_NAME, view);
    }

    inline bool write_schema_dst_file(View view)
    {
        return write_dst_file(LD_ASSET_DEFAULT_SCHEMA_FILE_KEY, LD_ASSET_DEFAULT_SCHEMA_FILE_NAME, view);
    }

    /// @brief Submit to job system, after which main thread should not
    ///        access member fields until job completion.
    void submit();

    /// @brief Direct synchronous execution on calling thread.
    ///        Does not leverage the job system.
    void execute_synchronous();
};

} // namespace LD