#pragma once

#include <LudensBuilder/AssetBuilder/AssetBuilderDef.h>

namespace LD {

struct MeshAssetImportInfo : AssetImportInfo
{
    FS::Path srcPath; /// path to load the source model format
};

void mesh_asset_import(void*);

} // namespace LD