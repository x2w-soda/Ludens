#pragma once

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