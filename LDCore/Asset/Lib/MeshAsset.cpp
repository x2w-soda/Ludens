#include "AssetObj.h"
#include <Ludens/Asset/MeshAsset.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/RenderComponent/Layout/RMesh.h>
#include <Ludens/System/FileSystem.h>

namespace LD {

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
    return mObj->modelBinary;
}

void MeshAssetImportJob::submit()
{
    mHeader.type = 0;
    mHeader.user = this;
    mHeader.fn = &MeshAssetImportJob::execute;

    JobSystem js = JobSystem::get();
    js.submit(&mHeader, JOB_DISPATCH_STANDARD);
}

void MeshAssetImportJob::execute(void* user)
{
    auto& self = *(MeshAssetImportJob*)user;
    MeshAssetObj* obj = self.asset.unwrap();

    std::string sourcePath = self.info.sourcePath.string();
    Model model = Model::load_gltf_model(sourcePath.c_str());
    model.apply_node_transform();

    obj->modelBinary = heap_new<ModelBinary>(MEMORY_USAGE_ASSET);
    obj->modelBinary->from_rigid_mesh(model);

    // save asset to disk
    Serializer serializer;
    ModelBinary::serialize(serializer, *obj->modelBinary);

    size_t binarySize;
    const byte* binary = serializer.view(binarySize);
    FS::write_file(self.info.savePath, binarySize, binary);
}

void MeshAssetLoadJob::submit()
{
    mHeader.type = 0;
    mHeader.user = this;
    mHeader.fn = &MeshAssetLoadJob::execute;

    JobSystem js = JobSystem::get();
    js.submit(&mHeader, JOB_DISPATCH_STANDARD);
}

void MeshAssetLoadJob::execute(void* user)
{
    LD_PROFILE_SCOPE;

    MeshAssetLoadJob& self = *(MeshAssetLoadJob*)user;
    MeshAssetObj* obj = self.asset.unwrap();

    obj->modelBinary = heap_new<ModelBinary>(MEMORY_USAGE_ASSET);

    uint64_t binarySize = FS::get_file_size(self.loadPath);
    if (binarySize == 0)
        return;

    Serializer serializer(binarySize);
    FS::read_file(self.loadPath, binarySize, serializer.data());
    ModelBinary::deserialize(serializer, *obj->modelBinary);
}

} // namespace LD