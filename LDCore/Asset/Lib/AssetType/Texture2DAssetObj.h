#pragma once

namespace LD {

/// @brief Texture2D asset implementation.
struct Texture2DAssetObj : AssetObj
{
    RSamplerInfo samplerHint;
    TextureCompression compression;
    Bitmap bitmap;

    static void load(void* assetLoadJob);
    static void unload(AssetObj* base);
};

} // namespace LD