#pragma once

#include <LudensBuilder/AssetBuilder/AssetImportInfo.h>

namespace LD {

struct UITemplateAssetImportInfo : AssetImportInfo
{
    FS::Path srcPath; /// path to load the source UITemplateSchema file
};

void ui_template_asset_copy_import_info(AssetImportInfoStorage& dstInfo, const AssetImportInfo* srcInfo);
void ui_template_asset_import(void*);

} // namespace LD