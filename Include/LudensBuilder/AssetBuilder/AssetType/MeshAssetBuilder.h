#pragma once

#include <LudensBuilder/AssetBuilder/AssetImportInfo.h>

namespace LD {

struct MeshAssetImportInfo : AssetImportInfo
{
    FS::Path srcPath; /// path to load the source model format
};

void mesh_asset_copy_import_info(AssetImportInfoStorage& dstInfo, const AssetImportInfo* srcInfo);
void mesh_asset_import(void*);

} // namespace LD