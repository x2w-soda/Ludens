#include <Ludens/Asset/AssetType/MeshAsset.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/RenderComponent/Layout/RMesh.h>
#include <Ludens/System/FileSystem.h>

#include "../AssetObj.h"
#include "MeshAssetObj.h"

namespace LD {

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

void MeshAssetImportJob::submit()
{
    mHeader.type = 0;
    mHeader.user = this;
    mHeader.onExecute = &MeshAssetImportJob::execute;

    JobSystem js = JobSystem::get();
    js.submit(&mHeader, JOB_DISPATCH_STANDARD);
}

void MeshAssetImportJob::execute(void* user)
{
    auto& self = *(MeshAssetImportJob*)user;
    auto* obj = (MeshAssetObj*)self.asset.unwrap();

    std::string sourcePath = self.info.sourcePath.string();
    Model model = Model::load_gltf_model(sourcePath.c_str());
    model.apply_node_transform();

    obj->modelBinary = heap_new<ModelBinary>(MEMORY_USAGE_ASSET);
    obj->modelBinary->from_rigid_mesh(model);

    // save asset to disk
    Serializer serializer;
    asset_header_write(serializer, ASSET_TYPE_MESH);

    ModelBinary::serialize(serializer, *obj->modelBinary);

    std::string err;
    bool ok = FS::write_file(self.info.savePath, serializer.view(), err);
    LD_ASSERT(ok); // TODO:
}

} // namespace LD