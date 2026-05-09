#include <Ludens/Asset/AssetType/LuaScriptAssetObj.h>
#include <Ludens/DSA/ViewUtil.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Serial/Serial.h>
#include <LudensBuilder/AssetBuilder/AssetState/LuaScriptAssetState.h>

#include "../AssetBuilderLibDef.h"
#include "../AssetImportJob.h"

namespace LD {

void lua_script_asset_import(void* user)
{
    auto& job = *(AssetImportJob*)user;
    auto* obj = (LuaScriptAssetObj*)job.asset.unwrap();
    const auto& info = *(LuaScriptAssetImportInfo*)job.info;

    std::string srcPath = info.srcPath.string();

    obj->sourcePath.clear();
    obj->source.clear();
    obj->domain = info.domain;

    // load source code from disk or RAM
    if (!srcPath.empty())
    {
        if (!job.read_src_file_to_string(info.srcPath, obj->source))
            return;
    }
    else
    {
        obj->source = info.srcCode;
    }

    FS::Path sourceRelPath = FS::Path(info.dstPath.c_str()).stem();
    sourceRelPath.replace_extension(".lua");

    obj->sourcePath = sourceRelPath.string();

    if (!job.write_dst_file("source", obj->sourcePath.c_str(), view(obj->source)))
        return;

    Serializer serial;
    asset_header_write(serial, ASSET_TYPE_LUA_SCRIPT);

    serial.write_chunk_begin("META");
    serial.write_u32((uint32_t)obj->domain);
    serial.write_chunk_end();

    (void)job.write_binary_dst_file(serial.view());
}

bool lua_script_asset_create(AssetCreateInfo* createInfo, String& err)
{
    auto* data = (LuaScriptAssetCreateData*)createInfo;

    switch (data->info.domain)
    {
    case LUA_SCRIPT_DOMAIN_COMPONENT:
        data->lua = "local comp = {} return {}";
        break;
    case LUA_SCRIPT_DOMAIN_GENERAL:
        data->lua = "local x = 3.0";
        break;
    default:
        err = "unknown LuaScriptDomain";
        return false;
    }

    return true;
}

bool lua_script_asset_prepare_import(const AssetCreateInfo* createInfo, AssetImportInfo* importInfo, String& err)
{
    const auto* data = (const LuaScriptAssetCreateData*)createInfo;
    auto* importI = (LuaScriptAssetImportInfo*)importInfo;
    importI->domain = data->info.domain;
    importI->srcPath.clear();
    importI->srcCode = data->lua;

    return true;
}

} // namespace LD