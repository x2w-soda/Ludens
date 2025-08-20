#include "AssetObj.h"
#include <Ludens/Asset/LuaScriptAsset.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/System/FileSystem.h>

namespace fs = std::filesystem;

namespace LD {

/// @brief Get asset ID.
AUID LuaScriptAsset::auid() const
{
    return mObj->auid;
}

/// @brief Unload asset from RAM.
void LuaScriptAsset::unload()
{
    if (mObj->source)
        heap_free(mObj->source);
}

/// @brief Get Lua script source string.
const char* LuaScriptAsset::get_source()
{
    LD_ASSERT(mObj->source);

    return mObj->source;
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