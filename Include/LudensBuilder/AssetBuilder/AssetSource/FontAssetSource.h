#pragma once

#include <Ludens/Header/View.h>
#include <LudensBuilder/AssetBuilder/AssetBuilderDef.h>

namespace LD {

struct FontAssetImportInfo : AssetImportInfo
{
    View srcView = {};      /// if not null, the font data in memory.
    FS::Path srcFile;       /// path to load the source format, used if srcView is null.
    float fontSize = 36.0f; /// desired font size for generating font atlas

    FontAssetImportInfo()
        : AssetImportInfo(ASSET_TYPE_FONT) {}
};

void font_asset_import(void*);
bool font_asset_import_info_set_src_files(AssetImportInfo* info, size_t count, const FS::Path* srcFilePaths);

} // namespace LD