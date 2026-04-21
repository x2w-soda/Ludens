#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/Media/AudioData.h>

namespace LD {

/// @brief Audio clip asset implementation.
struct AudioClipAssetObj : AssetObj
{
    AudioData data = {};

    bool load_from_binary(AssetLoadJob& job, const FS::Path& filePath);

    static void create(AssetObj* base);
    static void destroy(AssetObj* base);
    static void load(void* assetLoadJob);
    static void unload(AssetObj* base);
};

} // namespace LD