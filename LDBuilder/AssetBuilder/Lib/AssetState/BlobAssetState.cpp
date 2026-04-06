#include <Ludens/Asset/AssetType/BlobAssetObj.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Serial/Serial.h>
#include <LudensBuilder/AssetBuilder/AssetSource.h>
#include <LudensBuilder/AssetBuilder/AssetState/BlobAssetState.h>

#include "../AssetImportJob.h"

namespace LD {

static void try_import(AssetImportJob& job, BlobAssetObj* obj, const BlobAssetImportInfo info)
{
    if (info.srcView)
    {
        obj->dataSize = info.srcView.size;
        obj->data = heap_malloc(obj->dataSize, MEMORY_USAGE_ASSET);
        memcpy(obj->data, info.srcView.data, info.srcView.size);
    }
    else // load blob from disk
    {
        size_t fileSize;
        if (!FS::get_file_size(info.srcPath, fileSize, job.status.str))
        {
            job.status.type = ASSET_IMPORT_ERROR_SRC_PATH;
            return;
        }

        obj->dataSize = (uint64_t)fileSize;
        obj->data = heap_malloc(obj->dataSize, MEMORY_USAGE_ASSET);

        if (!FS::read_file(info.srcPath, MutView((char*)obj->data, obj->dataSize), job.status.str))
        {
            job.status.type = ASSET_IMPORT_ERROR_SRC_PATH;
            return;
        }
    }

    // save asset to disk
    Serializer serializer;
    asset_header_write(serializer, ASSET_TYPE_BLOB);

    serializer.write_u64(obj->dataSize);
    serializer.write((const byte*)obj->data, (size_t)obj->dataSize);

    job.write_to_dst_path(serializer.view());
}

void blob_asset_import(void* user)
{
    auto& job = *(AssetImportJob*)user;
    auto* obj = (BlobAssetObj*)job.asset.unwrap();
    const auto& info = *(BlobAssetImportInfo*)job.info;

    try_import(job, obj, info);

    if (job.status)
        return;

    if (obj->data)
    {
        heap_free(obj->data);
        obj->data = nullptr;
    }
}

} // namespace LD