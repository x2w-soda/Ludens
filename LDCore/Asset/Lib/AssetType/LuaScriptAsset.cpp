#include <Ludens/Asset/AssetManager.h>
#include <Ludens/Asset/AssetType/LuaScriptAsset.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Serial/Serial.h>
#include <Ludens/System/FileSystem.h>

#include "../AssetObj.h"
#include "LuaScriptAssetObj.h"

#include <vector>

namespace LD {

void LuaScriptAssetObj::load(void* user)
{
    LD_PROFILE_SCOPE;

    auto& job = *(AssetLoadJob*)user;
    LuaScriptAssetObj* obj = (LuaScriptAssetObj*)job.assetHandle.unwrap();

    std::string err; // TODO:
    std::vector<byte> file;
    if (!FS::read_file_to_vector(job.loadPath, file, err))
        return;

    Deserializer serial(file.data(), file.size());

    AssetType type;
    uint16_t major, minor, patch;
    if (!asset_header_read(serial, major, minor, patch, type))
        return;

    if (type != ASSET_TYPE_LUA_SCRIPT)
        return;

    std::string chunkName;
    chunkName.resize(4);
    uint32_t chunkSize;
    const byte* chunkData;

    while ((chunkData = serial.read_chunk(chunkName.data(), chunkSize)))
    {
        if (chunkName == "META")
        {
            uint32_t domainU32;
            serial.read_u32(domainU32);
            obj->domain = (LuaScriptDomain)domainU32;
            continue;
        }
    }

    FS::Path sourcePath = job.loadPath.replace_extension(".lua");
    uint64_t fileSize;
    if (!FS::get_file_size(sourcePath, fileSize, err) || fileSize == 0)
        return;

    obj->sourcePath = heap_strdup(sourcePath.string().c_str(), MEMORY_USAGE_ASSET);
    obj->source = (char*)heap_malloc(fileSize + 1, MEMORY_USAGE_ASSET);
    if (!FS::read_file(sourcePath, MutView(obj->source, fileSize), err))
    {
        heap_free(obj->source);
        obj->source = nullptr;
        return;
    }
    obj->source[fileSize] = '\0';
}

void LuaScriptAssetObj::unload(AssetObj* base)
{
    LuaScriptAssetObj& self = *(LuaScriptAssetObj*)base;

    if (self.sourcePath)
    {
        heap_free((void*)self.sourcePath);
        self.sourcePath = nullptr;
    }

    if (self.source)
    {
        heap_free((void*)self.source);
        self.source = nullptr;
    }
}

void LuaScriptAsset::unload()
{
    LuaScriptAssetObj::unload(mObj);

    mObj->manager->free_asset(mObj);
    mObj = nullptr;
}

FS::Path LuaScriptAsset::get_source_path()
{
    auto* obj = (LuaScriptAssetObj*)mObj;
    LD_ASSERT(obj->sourcePath);

    return FS::Path(obj->sourcePath);
}

const char* LuaScriptAsset::get_source()
{
    auto* obj = (LuaScriptAssetObj*)mObj;
    LD_ASSERT(obj->source);

    return obj->source;
}

void LuaScriptAsset::set_source(const char* src, size_t len)
{
    auto* obj = (LuaScriptAssetObj*)mObj;
    LD_ASSERT(obj->source);

    if (obj->source)
        heap_free(obj->source);

    obj->source = (char*)heap_malloc(len + 1, MEMORY_USAGE_ASSET);
    memcpy(obj->source, src, len);
    obj->source[len] = '\0';
}

void LuaScriptAssetImportJob::submit()
{
    mHeader.type = 0;
    mHeader.user = this;
    mHeader.fn = &LuaScriptAssetImportJob::execute;

    JobSystem js = JobSystem::get();
    js.submit(&mHeader, JOB_DISPATCH_STANDARD);
}

void LuaScriptAssetImportJob::execute(void* user)
{
    auto& self = *(LuaScriptAssetImportJob*)user;
    auto* obj = (LuaScriptAssetObj*)self.asset.unwrap();

    obj->sourcePath = heap_strdup(self.info.sourcePath.string().c_str(), MEMORY_USAGE_ASSET);
    obj->source = nullptr;
    obj->domain = self.info.domain;

    // source path is used only during this import process
    std::vector<byte> file;
    std::string err; // TODO:
    if (FS::read_file_to_vector(self.info.sourcePath, file, err))
    {
        obj->source = heap_strdup((const char*)file.data(), MEMORY_USAGE_ASSET);
    }

    Serializer serial;
    asset_header_write(serial, ASSET_TYPE_LUA_SCRIPT);

    serial.write_chunk_begin("META");
    serial.write_u32((uint32_t)obj->domain);
    serial.write_chunk_end();

    bool ok = FS::write_file(self.info.savePath, serial.view(), err);
    LD_ASSERT(ok);
}

} // namespace LD