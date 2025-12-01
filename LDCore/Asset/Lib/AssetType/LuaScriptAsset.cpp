#include "../AssetObj.h"
#include <Ludens/Asset/AssetManager.h>
#include <Ludens/Asset/AssetType/LuaScriptAsset.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/System/FileSystem.h>

namespace fs = std::filesystem;

namespace LD {

void LuaScriptAssetObj::load(void* user)
{
    LD_PROFILE_SCOPE;

    auto& job = *(AssetLoadJob*)user;
    LuaScriptAssetObj* obj = (LuaScriptAssetObj*)job.assetHandle.unwrap();

    uint64_t fileSize = FS::get_file_size(job.loadPath);
    if (fileSize == 0)
        return;

    obj->source = (char*)heap_malloc(fileSize + 1, MEMORY_USAGE_ASSET);
    FS::read_file(job.loadPath, fileSize, (byte*)obj->source);
    obj->source[fileSize] = '\0';
}

void LuaScriptAssetObj::unload(AssetObj* base)
{
    LuaScriptAssetObj& self = *(LuaScriptAssetObj*)base;

    if (self.source)
        heap_free((void*)self.source);
}

void LuaScriptAsset::unload()
{
    LuaScriptAssetObj::unload(mObj);

    mObj->manager->free_asset(mObj);
    mObj = nullptr;
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

} // namespace LD