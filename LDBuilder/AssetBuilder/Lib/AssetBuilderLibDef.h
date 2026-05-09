#pragma once

#include <LudensBuilder/AssetBuilder/AssetBuilderDef.h>

// Module Private Header.

namespace LD {

struct LuaScriptAssetCreateData
{
    LuaScriptAssetCreateInfo info;
    String lua;
};

void lua_script_asset_import(void*);
bool lua_script_asset_create(AssetCreateInfo* createInfo, String& err);
bool lua_script_asset_prepare_import(const AssetCreateInfo* createInfo, AssetImportInfo* importInfo, String& err);

} // namespace LD