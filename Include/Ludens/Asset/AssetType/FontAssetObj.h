#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/Media/Font.h>

namespace LD {

/// @brief Font asset implementation.
struct FontAssetObj : AssetObj
{
    Font font;
    FontAtlas fontAtlas;
    float fontSize;

    static void load(void* assetLoadJob);
    static void unload(AssetObj* base);
};

} // namespace LD