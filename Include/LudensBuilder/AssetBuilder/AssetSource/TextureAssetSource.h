#pragma once

#include <Ludens/RenderBackend/RBackend.h>
#include <LudensBuilder/AssetBuilder/AssetSource.h>

namespace LD {

struct Texture2DAssetImportInfo : AssetImportInfo
{
    FS::Path srcPath;         /// absolute path to load the source texture
    RSamplerInfo samplerHint; /// desired texture sampler mode
};

} // namespace LD