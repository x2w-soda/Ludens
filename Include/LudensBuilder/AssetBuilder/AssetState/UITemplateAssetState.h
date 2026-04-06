#pragma once

#include <LudensBuilder/AssetBuilder/AssetSource.h>

namespace LD {

struct UITemplateAssetImportInfo : AssetImportInfo
{
    FS::Path srcPath; /// path to load the source UITemplateSchema file
};

void ui_template_asset_import(void*);

} // namespace LD