#pragma once

#include <Ludens/RenderBackend/RBackend.h>
#include <LudensBuilder/AssetBuilder/AssetBuilderDef.h>

namespace LD {

struct Texture2DAssetImportInfo : AssetImportInfo
{
    FS::Path srcFile;              /// absolute path to load the source texture
    RSamplerInfo samplerHint = {}; /// desired texture sampler mode

    Texture2DAssetImportInfo()
        : AssetImportInfo(ASSET_TYPE_TEXTURE_2D) {}
};

void texture_2d_asset_import(void*);
bool texture_2d_asset_import_info_set_src_files(AssetImportInfo* info, size_t count, const FS::Path* srcFilePaths);

} // namespace LD