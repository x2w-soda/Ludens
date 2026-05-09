#pragma once

#include <Ludens/Asset/AssetType/LuaScriptAsset.h>

namespace LD {

/// @brief Lua script asset implementation. Should contain enough information
///        to instantiate lua script instances.
struct LuaScriptAssetObj : AssetObj
{
    String sourcePath;           /// lua script file path
    String source;               /// lua source code string, if found
    LuaScriptDomain domain = {}; /// intended script domain

    bool load_from_binary(AssetLoadJob& job, const FS::Path& filePath);

    static void create(AssetObj* base);
    static void destroy(AssetObj* base);
    static void load(void* assetLoadJob);
    static void unload(AssetObj* base);
};

} // namespace LD