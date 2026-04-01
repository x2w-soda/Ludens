#pragma once

#include <Ludens/Asset/AssetType/LuaScriptAsset.h>

namespace LD {

/// @brief Lua script asset implementation. Should contain enough information
///        to instantiate lua script instances.
struct LuaScriptAssetObj : AssetObj
{
    char* sourcePath; /// lua script file path
    char* source;     /// lua source code string, if found
    LuaScriptDomain domain;

    static void load(void* assetLoadJob);
    static void unload(AssetObj* base);
};

} // namespace LD