#include <Ludens/Asset/AssetType/BlobAsset.h>
#include <Ludens/Asset/AssetType/BlobAssetObj.h>
#include <Ludens/Profiler/Profiler.h>

#include "../AssetMeta.h"

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

} // namespace LD