#include <Ludens/Asset/Asset.h>
#include <Ludens/Asset/AssetManager.h>
#include <Ludens/Asset/AssetType/LuaScriptAsset.h>
#include <Ludens/Asset/AssetType/LuaScriptAssetObj.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Serial/Serial.h>
#include <Ludens/System/FileSystem.h>

#include "../AssetLoadJob.h"
#include "../AssetMeta.h"

namespace LD {

// TODO: this isn't really binary, source code should be baked into LuaJIT bytecode
bool LuaScriptAssetObj::load_from_binary(AssetLoadJob& job, const FS::Path& filePath)
{
    Vector<byte> file;
    if (!job.read_file_to_vector(filePath, file))
        return false;

    Deserializer serial(file.data(), file.size());

    AssetType type;
    uint16_t major, minor, patch;
    if (!asset_header_read(serial, major, minor, patch, type))
        return false;

    if (type != ASSET_TYPE_LUA_SCRIPT)
        return false;

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
            domain = (LuaScriptDomain)domainU32;
            continue;
        }
    }

    FS::Path sourcePath = job.assetDirPath / FS::Path(job.assetEntry.get_file_path("source"));
    if (!job.read_file_to_str(sourcePath, source))
        return false;

    return true;
}

void LuaScriptAssetObj::create(AssetObj* base)
{
    new (base) LuaScriptAssetObj();
}

void LuaScriptAssetObj::destroy(AssetObj* base)
{
    ((LuaScriptAssetObj*)base)->~LuaScriptAssetObj();
}

void LuaScriptAssetObj::load(void* user)
{
    LD_PROFILE_SCOPE;

    auto& job = *(AssetLoadJob*)user;
    LuaScriptAssetObj* obj = (LuaScriptAssetObj*)job.assetHandle.unwrap();

    FS::Path filePath = job.assetDirPath / LD_ASSET_DEFAULT_BINARY_FILE_NAME;
    if (FS::exists(filePath))
        obj->load_from_binary(job, filePath);

    // TODO:
}

void LuaScriptAssetObj::unload(AssetObj* base)
{
    (void)base;
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
    LD_ASSERT(!obj->sourcePath.empty());

    return FS::Path(obj->sourcePath);
}

View LuaScriptAsset::get_source()
{
    auto* obj = (LuaScriptAssetObj*)mObj;
    LD_ASSERT(!obj->source.empty());

    return View(obj->source.data(), obj->source.size());
}

void LuaScriptAsset::set_source(View source)
{
    auto* obj = (LuaScriptAssetObj*)mObj;

    obj->source.resize(source.size);
    memcpy(obj->source.data(), source.data, source.size);
}

} // namespace LD