#include <Ludens/Asset/AssetType/MeshAssetObj.h>
#include <Ludens/Media/Model.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Serial/Serial.h>
#include <LudensBuilder/AssetBuilder/AssetState/MeshAssetState.h>

#include "../AssetImportJob.h"

namespace LD {
#if 0
void mesh_asset_import(void* user)
{
    LD_PROFILE_SCOPE;

    auto& job = *(AssetImportJob*)user;
    auto* obj = (MeshAssetObj*)job.asset.unwrap();
    const auto& info = *(MeshAssetImportInfo*)job.info;

    std::string sourcePath = info.srcPath.string();
    Model model = Model::load_gltf_model(sourcePath.c_str());
    if (!job.require(model, "failed to load gltf model"))
        return;

    model.apply_node_transform();

    obj->modelBinary = heap_new<ModelBinary>(MEMORY_USAGE_ASSET);
    obj->modelBinary->from_rigid_mesh(model);

    // save asset to disk
    Serializer serializer;
    asset_header_write(serializer, ASSET_TYPE_MESH);

    ModelBinary::serialize(serializer, *obj->modelBinary);

    job.write_to_dst_file(serializer.view());
}
#endif
} // namespace LD