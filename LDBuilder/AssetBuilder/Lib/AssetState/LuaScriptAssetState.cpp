#include <Ludens/Asset/AssetType/LuaScriptAssetObj.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Serial/Serial.h>
#include <LudensBuilder/AssetBuilder/AssetState/LuaScriptAssetState.h>

#include "../AssetImportJob.h"

namespace LD {

void lua_script_asset_import(void* user)
{
    auto& job = *(AssetImportJob*)user;
    auto* obj = (LuaScriptAssetObj*)job.asset.unwrap();
    const auto& info = *(LuaScriptAssetImportInfo*)job.info;

    obj->sourcePath = heap_strdup(info.srcPath.string().c_str(), MEMORY_USAGE_ASSET);
    obj->source = nullptr;
    obj->domain = info.domain;

    // source path is used only during this import process
    Vector<byte> file;
    if (FS::read_file_to_vector(info.srcPath, file, job.status.str))
    {
        obj->source = heap_strdup((const char*)file.data(), MEMORY_USAGE_ASSET);
    }

    Serializer serial;
    asset_header_write(serial, ASSET_TYPE_LUA_SCRIPT);

    serial.write_chunk_begin("META");
    serial.write_u32((uint32_t)obj->domain);
    serial.write_chunk_end();

    job.write_to_dst_path(serial.view());
}

} // namespace LD