#pragma once

#include <LudensBuilder/AssetBuilder/AssetImportInfo.h>

namespace LD {

struct Texture2DAssetImportInfo : AssetImportInfo
{
    FS::Path srcPath;         /// absolute path to load the source texture
    RSamplerInfo samplerHint; /// desired texture sampler mode
};

void texture_2d_asset_copy_import_info(AssetImportInfoStorage& dstInfo, const AssetImportInfo* srcInfo);
void texture_2d_asset_import(void*);

} // namespace LD