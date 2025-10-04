#include "AssetObj.h"
#include <Ludens/Asset/AssetManager.h>
#include <Ludens/Asset/LuaScriptAsset.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/System/FileSystem.h>

namespace fs = std::filesystem;

namespace LD {

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
    LD_ASSERT(mObj->source);

    return mObj->source;
}

void LuaScriptAsset::set_source(const char* src, size_t len)
{
    LD_ASSERT(mObj->source);

    if (mObj->source)
        heap_free(mObj->source);

    mObj->source = (char*)heap_malloc(len + 1, MEMORY_USAGE_ASSET);
    memcpy(mObj->source, src, len);
    mObj->source[len] = '\0';
}

void LuaScriptAssetLoadJob::submit()
{
    mHeader.user = this;
    mHeader.type = 0;
    mHeader.fn = &LuaScriptAssetLoadJob::execute;

    JobSystem::get().submit(&mHeader, JOB_DISPATCH_STANDARD);
}

void LuaScriptAssetLoadJob::execute(void* user)
{
    LD_PROFILE_SCOPE;

    auto& self = *(LuaScriptAssetLoadJob*)user;
    LuaScriptAssetObj* obj = self.asset.unwrap();

    uint64_t fileSize = FS::get_file_size(self.loadPath);
    if (fileSize == 0)
        return;

    obj->source = (char*)heap_malloc(fileSize + 1, MEMORY_USAGE_ASSET);
    FS::read_file(self.loadPath, fileSize, (byte*)obj->source);
    obj->source[fileSize] = '\0';
}

} // namespace LD