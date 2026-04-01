#pragma once

#include <Ludens/Asset/AssetType/LuaScriptAsset.h>
#include <LudensBuilder/AssetBuilder/AssetImportInfo.h>

namespace LD {

struct LuaScriptAssetImportInfo : AssetImportInfo
{
    FS::Path srcPath; /// path to import the lua script file
    LuaScriptDomain domain = LUA_SCRIPT_DOMAIN_COMPONENT;
};

void lua_script_asset_copy_import_info(AssetImportInfoStorage& dstInfo, const AssetImportInfo* srcInfo);
void lua_script_asset_import(void*);

} // namespace LD