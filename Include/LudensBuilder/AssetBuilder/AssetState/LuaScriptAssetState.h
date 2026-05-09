#pragma once

#include <Ludens/Asset/AssetType/LuaScriptAsset.h>
#include <LudensBuilder/AssetBuilder/AssetBuilderDef.h>

namespace LD {

/// @brief Info to import lua script data into project.
struct LuaScriptAssetImportInfo : AssetImportInfo
{
    FS::Path srcPath; /// path to import the lua script file
    String srcCode;   /// if not empty, the lua source code is used instead of file path.
    LuaScriptDomain domain = LUA_SCRIPT_DOMAIN_COMPONENT;

    LuaScriptAssetImportInfo()
        : AssetImportInfo(ASSET_TYPE_LUA_SCRIPT) {}
};

/// @brief Info to create lua script data for import.
struct LuaScriptAssetCreateInfo : AssetCreateInfo
{
    LuaScriptDomain domain = LUA_SCRIPT_DOMAIN_COMPONENT;

    LuaScriptAssetCreateInfo()
        : AssetCreateInfo(ASSET_CREATE_TYPE_LUA_SCRIPT, ASSET_TYPE_LUA_SCRIPT) {}
};

} // namespace LD