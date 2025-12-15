#pragma once

namespace LD {

/// @brief Lua script asset implementation. Should contain enough information
///        to instantiate lua script instances.
struct LuaScriptAssetObj : AssetObj
{
    char* source; /// lua source code string

    static void load(void* assetLoadJob);
    static void unload(AssetObj* base);
};

} // namespace LD