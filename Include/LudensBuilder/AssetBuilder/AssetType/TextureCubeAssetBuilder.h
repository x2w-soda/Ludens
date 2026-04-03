#pragma once

#include <LudensBuilder/AssetBuilder/AssetImportInfo.h>

namespace LD {

struct TextureCubeAssetImportInfo : AssetImportInfo
{
    RSamplerInfo samplerHint = {}; /// desired texture sampler mode, applied to all faces
    FS::Path srcPaths[6];          /// path to load the faces
};

void texture_cube_asset_import(void*);

} // namespace LD