#pragma once

namespace LD {

// NOTE: Placeholder Mesh asset implementation.
//       Need to figure out texture and material assets first.
struct MeshAssetObj : AssetObj
{
    ModelBinary* modelBinary;

    static void load(void* assetLoadJob);
    static void unload(AssetObj* base);
};

} // namespace LD