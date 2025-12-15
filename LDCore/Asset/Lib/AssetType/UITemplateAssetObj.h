#pragma once

#include "../AssetObj.h"

namespace LD {

/// @brief UITemplateAsset implementation
struct UITemplateAssetObj : AssetObj
{
    UITemplate tmpl;
    // TODO: AUID luaScriptAUID;

    static void load(void* assetLoadJob);
    static void unload(AssetObj* base);
};

} // namespace LD