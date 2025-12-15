#include <Ludens/Asset/AssetType/BlobAsset.h>
#include <Ludens/Profiler/Profiler.h>

#include "../AssetObj.h"
#include "BlobAssetObj.h"

namespace LD {

void BlobAssetObj::load(void* assetLoadJob)
{
    LD_PROFILE_SCOPE;

    auto& job = *(AssetLoadJob*)assetLoadJob;
    auto* obj = (BlobAssetObj*)job.assetHandle.unwrap();

    uint64_t fileSize = FS::get_file_size(job.loadPath);
    if (fileSize == 0)
        return;

    obj->fileData = heap_malloc(fileSize, MEMORY_USAGE_ASSET);
    if (!FS::read_file(job.loadPath, fileSize, (byte*)obj->data))
        return;

    Deserializer serial(obj->fileData, fileSize);

    AssetType type;
    uint16_t major, minor, patch;
    if (!asset_header_read(serial, major, minor, patch, type))
        return;

    if (type != ASSET_TYPE_BLOB)
        return;

    serial.read_u64(obj->dataSize);
    obj->data = (void*)serial.view_now();
}

void BlobAssetObj::unload(AssetObj* base)
{
    auto* obj = (BlobAssetObj*)base;

    heap_free(obj->fileData);
    obj->fileData = nullptr;
    obj->data = nullptr;
    obj->dataSize = 0;
}

void* BlobAsset::get_data(size_t& dataSize)
{
    auto* obj = (BlobAssetObj*)mObj;

    dataSize = (size_t)obj->dataSize;
    return obj->data;
}

void BlobAssetImportJob::submit()
{
    mHeader.type = 0;
    mHeader.user = this;
    mHeader.fn = &BlobAssetImportJob::execute;

    JobSystem js = JobSystem::get();
    js.submit(&mHeader, JOB_DISPATCH_STANDARD);
}

void BlobAssetImportJob::execute(void* user)
{
    auto& self = *(BlobAssetImportJob*)user;
    auto* obj = (BlobAssetObj*)self.asset.unwrap();

    if (self.info.sourceData)
    {
        obj->dataSize = self.info.sourceDataSize;
        obj->data = heap_malloc(obj->dataSize, MEMORY_USAGE_ASSET);
        memcpy(obj->data, self.info.sourceData, self.info.sourceDataSize);
    }
    else
    {
        size_t fileSize = FS::get_file_size(self.info.sourcePath);
        obj->dataSize = (uint64_t)fileSize;
        obj->data = heap_malloc(obj->dataSize, MEMORY_USAGE_ASSET);
        bool ok = FS::read_file(self.info.sourcePath, obj->dataSize, (byte*)obj->data);
    }

    // save asset to disk
    Serializer serializer;
    asset_header_write(serializer, ASSET_TYPE_BLOB);

    serializer.write_u64(obj->dataSize);
    serializer.write((const byte*)obj->data, (size_t)obj->dataSize);

    size_t binarySize;
    const byte* binary = serializer.view(binarySize);
    FS::write_file(self.info.savePath, binarySize, binary);
}

} // namespace LD