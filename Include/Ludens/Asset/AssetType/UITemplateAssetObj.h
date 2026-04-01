#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/Asset/Template/UITemplate.h>

namespace LD {

/// @brief UITemplateAsset implementation
struct UITemplateAssetObj : AssetObj
{
    UITemplate tmpl;
    char* luaSource;

    static void load(void* assetLoadJob);
    static void unload(AssetObj* base);
};

} // namespace LD