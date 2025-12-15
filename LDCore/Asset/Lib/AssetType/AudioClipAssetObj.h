#pragma once

namespace LD {

/// @brief Audio clip asset implementation.
struct AudioClipAssetObj : AssetObj
{
    AudioData data;

    static void load(void* assetLoadJob);
    static void unload(AssetObj* base);
};

} // namespace LD