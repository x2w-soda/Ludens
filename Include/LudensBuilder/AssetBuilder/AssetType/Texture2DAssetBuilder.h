#pragma once

#include <Ludens/RenderBackend/RBackend.h>
#include <LudensBuilder/AssetBuilder/AssetImportInfo.h>

namespace LD {

struct Texture2DAssetImportInfo : AssetImportInfo
{
    FS::Path srcPath;         /// absolute path to load the source texture
    RSamplerInfo samplerHint; /// desired texture sampler mode
};

void texture_2d_asset_import(void*);

} // namespace LD