#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/JobSystem/JobSystem.h>
#include <Ludens/System/FileSystem.h>
#include <cstddef>

namespace LD {

/// @brief Blob asset handle. The engine makes no assumptions about the binary contents.
struct BlobAsset : Asset
{
    /// @brief Get blob data.
    void* get_data(size_t& dataSize);
};

struct BlobAssetImportInfo
{
    const void* sourceData = nullptr; /// if not null, the address of some blob in memory.
    size_t sourceDataSize;            /// byte size of blob in memory
    FS::Path sourcePath;              /// path to load the blob file, used if source data is null.
    FS::Path savePath;                /// path to save the imported asset
};

class BlobAssetImportJob
{
public:
    BlobAsset asset;          /// subject asset handle
    BlobAssetImportInfo info; /// Blob import configuration

    /// @brief Submit to job system. Address of this job instance must not
    ///        change until the worker thread completes execution.
    void submit();

private:
    static void execute(void*);

    JobHeader mHeader;
};

} // namespace LD