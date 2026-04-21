#include <Ludens/Asset/AssetType/MeshAsset.h>
#include <Ludens/Asset/AssetType/MeshAssetObj.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/RenderComponent/Layout/RMesh.h>
#include <Ludens/System/FileSystem.h>

#include "../AssetMeta.h"

namespace LD {
#if 0

void MeshAssetObj::load(void* user)
{
    LD_PROFILE_SCOPE;

    auto& job = *(AssetLoadJob*)user;
    MeshAssetObj* obj = (MeshAssetObj*)job.assetHandle.unwrap();

    obj->modelBinary = heap_new<ModelBinary>(MEMORY_USAGE_ASSET);

    std::string err; // TODO:
    std::vector<byte> tmp;
    if (!FS::read_file_to_vector(job.loadPath, tmp, err))
        return;

    Deserializer serial(tmp.data(), tmp.size());

    AssetType type;
    uint16_t major, minor, patch;
    if (!asset_header_read(serial, major, minor, patch, type))
        return;

    if (type != ASSET_TYPE_MESH)
        return;

    bool success = ModelBinary::deserialize(serial, *obj->modelBinary);
    LD_ASSERT(success);
}

void MeshAssetObj::unload(AssetObj* base)
{
    MeshAssetObj& self = *(MeshAssetObj*)base;

    if (self.modelBinary)
    {
        heap_delete<ModelBinary>(self.modelBinary);
        self.modelBinary = nullptr;
    }
}

void MeshAsset::unload()
{
    MeshAssetObj::unload(mObj);

    mObj->manager->free_asset(mObj);
    mObj = nullptr;
}

ModelBinary* MeshAsset::data()
{
    auto* obj = (MeshAssetObj*)mObj;

    return obj->modelBinary;
}

#endif
} // namespace LD