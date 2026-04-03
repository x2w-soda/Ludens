#pragma once

#include <Ludens/Header/View.h>
#include <LudensBuilder/AssetBuilder/AssetImportInfo.h>

namespace LD {

struct FontAssetImportInfo : AssetImportInfo
{
    View srcView = {};      /// if not null, the font data in memory.
    FS::Path srcPath;       /// path to load the source format, used if srcView is null.
    float fontSize = 36.0f; /// desired font size for generating font atlas
};

void font_asset_import(void*);

} // namespace LD