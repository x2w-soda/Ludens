#pragma once

#include <Ludens/Asset/AssetManager.h>
#include <Ludens/JobSystem/JobSystem.h>

#include <atomic>

namespace LD {

/// @brief Job context for loading an Asset.
/// @warning Address of this struct must not change since it is supplied as JobHeader::user,
///          that means worker threads will be accessing this struct.
struct AssetLoadJob
{
    JobHeader jobHeader;            /// submitted to the job system
    Asset assetHandle;              /// accessed by job thread, dst class handle
    AssetEntry assetEntry;          /// accessed by job thread, src asset to load
    AssetLoadStatus status;         /// load status
    FS::Path assetDirPath;          /// absolute path to asset directory
    std::atomic_bool jobInProgress; /// read by main thread
    std::atomic<float> jobProgress; /// read by main thread, normalized job progress estimate

    /// @brief Check predicate or update status with an error.
    inline bool require(bool pred, const char* str)
    {
        if (!pred)
        {
            status.str = str;
            status.type = ASSET_LOAD_ERROR;
            return false;
        }

        return true;
    }

    /// @brief Try reading file to vector, updates status upon failure.
    inline bool read_file_to_vector(const FS::Path& path, Vector<byte>& v)
    {
        if (!FS::read_file_to_vector(path, v, status.str))
        {
            status.type = ASSET_LOAD_ERROR_FILE_PATH;
            return false;
        }

        return true;
    }

    /// @brief Try reading file to std string, updates status upon failure.
    inline bool read_file_to_str(const FS::Path& path, std::string& str)
    {
        uint64_t fileSize;
        if (!FS::get_file_size(path, fileSize, status.str) || fileSize == 0)
        {
            status.type = ASSET_LOAD_ERROR_FILE_PATH;
            return false;
        }

        str.resize(fileSize);

        if (!FS::read_file(path, MutView(str.data(), str.size()), status.str))
        {
            status.type = ASSET_LOAD_ERROR_FILE_PATH;
            return false;
        }

        return true;
    }

    /// @brief Try reading file to mutable view, updates status upon failure.
    inline bool read_file(const FS::Path& path, MutView v)
    {
        if (!FS::read_file(path, v, status.str))
        {
            status.type = ASSET_LOAD_ERROR_FILE_PATH;
            return false;
        }

        return true;
    }

    /// @brief Try getting file size, updates status upon failure.
    inline bool get_file_size(const FS::Path& path, uint64_t& size)
    {
        if (!FS::get_file_size(path, size, status.str) || size == 0)
        {
            status.type = ASSET_LOAD_ERROR_FILE_PATH;
            return false;
        }

        return true;
    }
};

} // namespace LD