#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/Media/Font.h>
#include <Ludens/System/FileSystem.h>

namespace LD {

/// @brief Font asset implementation.
struct FontAssetObj : AssetObj
{
    Font font = {};
    FontAtlas fontAtlas = {};
    float fontSize = 0.0f;

    bool load_from_binary(AssetLoadJob& job, const FS::Path& filePath);

    static void create(AssetObj* base);
    static void destroy(AssetObj* base);
    static void load(void* assetLoadJob);
    static void unload(AssetObj* base);
};

} // namespace LD