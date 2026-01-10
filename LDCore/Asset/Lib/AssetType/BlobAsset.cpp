#include <Ludens/Asset/AssetType/BlobAsset.h>
#include <Ludens/Profiler/Profiler.h>

#include "../AssetObj.h"
#include "BlobAssetObj.h"

namespace LD {

void BlobAssetObj::load(void* assetLoadJob)
{
    LD_PROFILE_SCOPE;

    auto& loadJob = *(AssetLoadJob*)assetLoadJob;
    auto* obj = (BlobAssetObj*)loadJob.assetHandle.unwrap();
    obj->data = nullptr;

    DiagnosticScope scope(loadJob.diagnostics, "BlobAssetObj::load");

    uint64_t fileSize;
    if (!FS::get_positive_file_size(loadJob.loadPath, fileSize, loadJob.diagnostics))
        return;

    obj->fileData = heap_malloc(fileSize, MEMORY_USAGE_ASSET);
    if (!FS::read_file(loadJob.loadPath, MutView((char*)obj->fileData, fileSize), loadJob.diagnostics))
        return; // TODO: fix leak

    Deserializer serial(obj->fileData, fileSize);

    if (!asset_header_read(serial, ASSET_TYPE_BLOB, loadJob.diagnostics))
        return; // TODO: fix leak

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
    mHeader.onExecute = &BlobAssetImportJob::execute;

    JobSystem js = JobSystem::get();
    js.submit(&mHeader, JOB_DISPATCH_STANDARD);
}

void BlobAssetImportJob::execute(void* user)
{
    auto& self = *(BlobAssetImportJob*)user;
    auto* obj = (BlobAssetObj*)self.asset.unwrap();

    std::string err; // TODO:

    if (self.info.sourceData)
    {
        obj->dataSize = self.info.sourceDataSize;
        obj->data = heap_malloc(obj->dataSize, MEMORY_USAGE_ASSET);
        memcpy(obj->data, self.info.sourceData, self.info.sourceDataSize);
    }
    else
    {
        size_t fileSize;
        bool ok = FS::get_file_size(self.info.sourcePath, fileSize, err);
        obj->dataSize = (uint64_t)fileSize;
        obj->data = heap_malloc(obj->dataSize, MEMORY_USAGE_ASSET);
        ok = FS::read_file(self.info.sourcePath, MutView((char*)obj->data, obj->dataSize), err);
    }

    // save asset to disk
    Serializer serializer;
    asset_header_write(serializer, ASSET_TYPE_BLOB);

    serializer.write_u64(obj->dataSize);
    serializer.write((const byte*)obj->data, (size_t)obj->dataSize);

    View serialView = serializer.view();
    bool ok = FS::write_file(self.info.savePath, serialView, err);
    LD_ASSERT(ok); // TODO: asset import error
}

} // namespace LD