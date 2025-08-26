#include "AssetObj.h"
#include <Ludens/Asset/AssetManager.h>
#include <Ludens/Asset/MeshAsset.h>
#include <Ludens/Asset/TextureAsset.h>
#include <Ludens/Log/Log.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/RenderBackend/RUtil.h>
#include <Ludens/RenderComponent/Layout/RMaterial.h>
#include <Ludens/RenderComponent/Layout/RMesh.h>
#include <Ludens/System/Allocator.h>
#include <filesystem>
#include <string>
#include <unordered_map>

namespace fs = std::filesystem;

namespace LD {

static Log sLog("AssetManager");

// clang-format off
struct
{
    AssetType type;
    size_t size;
} sAssetTypeTable[] = {
    {ASSET_TYPE_MESH,       sizeof(MeshAssetObj)},
    {ASSET_TYPE_TEXTURE_2D, sizeof(Texture2DAssetObj)},
    {ASSET_TYPE_LUA_SCRIPT, sizeof(LuaScriptAssetObj)},
};
// clang-format on

static_assert(sizeof(sAssetTypeTable) / sizeof(*sAssetTypeTable) == ASSET_TYPE_ENUM_COUNT);

size_t get_asset_byte_size(AssetType type)
{
    return sAssetTypeTable[(int)type].size;
}

/// @brief Asset manager implementation.
class AssetManagerObj
{
public:
    AssetManagerObj() = delete;
    AssetManagerObj(const fs::path& rootPath);
    AssetManagerObj(const AssetManagerObj&) = delete;
    ~AssetManagerObj();

    AssetManagerObj& operator=(const AssetManagerObj&) = delete;

    void* allocate_asset(AssetType type);

    void begin_load_batch();
    void end_load_batch();

    void load_mesh_asset(const fs::path& path, AUID auid);
    void load_texture_2d_asset(const fs::path& path, AUID auid);
    void load_lua_script_asset(const fs::path& path, AUID auid);

    Texture2DAsset get_texture_2d_asset(AUID auid);
    MeshAsset get_mesh_asset(AUID auid);
    LuaScriptAsset get_lua_script_asset(AUID auid);

private:
    std::unordered_map<AssetType, PoolAllocator> mAllocators;
    std::unordered_map<AUID, MeshAsset> mMeshAssets;
    std::unordered_map<AUID, Texture2DAsset> mTexture2DAssets;
    std::unordered_map<AUID, LuaScriptAsset> mLuaScriptAssets;
    std::vector<MeshAssetLoadJob*> mMeshLoadJobs;
    std::vector<Texture2DAssetLoadJob*> mTexture2DLoadJobs;
    std::vector<LuaScriptAssetLoadJob*> mLuaScriptLoadJobs;
    const fs::path mRootPath;  /// asset URIs are relative paths to root path
    bool mInLoadBatch = false; /// is within load batch scope
};

AssetManagerObj::AssetManagerObj(const fs::path& rootPath)
    : mRootPath(rootPath)
{
}

AssetManagerObj::~AssetManagerObj()
{
    for (auto ite : mMeshAssets)
        ite.second.unload();

    for (auto ite : mTexture2DAssets)
        ite.second.unload();

    for (auto ite : mLuaScriptAssets)
        ite.second.unload();

    for (auto ite : mAllocators)
        PoolAllocator::destroy(ite.second);
}

void* AssetManagerObj::allocate_asset(AssetType type)
{
    if (!mAllocators.contains(type))
    {
        PoolAllocatorInfo paI{};
        paI.usage = MEMORY_USAGE_ASSET;
        paI.blockSize = get_asset_byte_size(type);
        paI.pageSize = 16;
        paI.isMultiPage = true;
        mAllocators[type] = PoolAllocator::create(paI);
    }

    return mAllocators[type].allocate();
}

void AssetManagerObj::begin_load_batch()
{
    LD_ASSERT(!mInLoadBatch);

    mInLoadBatch = true;

    mMeshLoadJobs.clear();
    mTexture2DLoadJobs.clear();
    mLuaScriptLoadJobs.clear();
}

void AssetManagerObj::end_load_batch()
{
    LD_ASSERT(mInLoadBatch);

    mInLoadBatch = false;

    // TODO: wait for asset load jobs only
    JobSystem::get().wait_all();

    for (MeshAssetLoadJob* job : mMeshLoadJobs)
    {
        MeshAssetObj* meshAssetObj = job->asset.unwrap();
        mMeshAssets[meshAssetObj->auid] = job->asset;
        heap_delete<MeshAssetLoadJob>(job);
    }

    for (Texture2DAssetLoadJob* job : mTexture2DLoadJobs)
    {
        Texture2DAssetObj* textureAssetObj = job->asset.unwrap();
        mTexture2DAssets[textureAssetObj->auid] = job->asset;
        heap_delete<Texture2DAssetLoadJob>(job);
    }

    for (LuaScriptAssetLoadJob* job : mLuaScriptLoadJobs)
    {
        LuaScriptAssetObj* scriptAssetObj = job->asset.unwrap();
        mLuaScriptAssets[scriptAssetObj->auid] = job->asset;
        heap_delete<LuaScriptAssetLoadJob>(job);
    }
}

void AssetManagerObj::load_mesh_asset(const fs::path& path, AUID auid)
{
    LD_ASSERT(mInLoadBatch);

    auto obj = (MeshAssetObj*)allocate_asset(ASSET_TYPE_MESH);
    obj->auid = auid;

    fs::path loadPath = mRootPath / path;

    auto meshLoadJob = heap_new<MeshAssetLoadJob>(MEMORY_USAGE_ASSET);
    meshLoadJob->asset = MeshAsset(obj);
    meshLoadJob->loadPath = loadPath;
    meshLoadJob->submit();
    mMeshLoadJobs.push_back(meshLoadJob);
}

void AssetManagerObj::load_texture_2d_asset(const fs::path& path, AUID auid)
{
    LD_ASSERT(mInLoadBatch);

    auto obj = (Texture2DAssetObj*)allocate_asset(ASSET_TYPE_TEXTURE_2D);
    obj->auid = auid;

    fs::path loadPath = mRootPath / path;

    auto textureLoadJob = heap_new<Texture2DAssetLoadJob>(MEMORY_USAGE_ASSET);
    textureLoadJob->asset = Texture2DAsset(obj);
    textureLoadJob->loadPath = loadPath;
    textureLoadJob->submit();
    mTexture2DLoadJobs.push_back(textureLoadJob);
}

void AssetManagerObj::load_lua_script_asset(const fs::path& path, AUID auid)
{
    LD_ASSERT(mInLoadBatch);

    auto obj = (LuaScriptAssetObj*)allocate_asset(ASSET_TYPE_LUA_SCRIPT);
    obj->auid = auid;

    fs::path loadPath = mRootPath / path;

    auto scriptLoadJob = heap_new<LuaScriptAssetLoadJob>(MEMORY_USAGE_ASSET);
    scriptLoadJob->asset = LuaScriptAsset(obj);
    scriptLoadJob->loadPath = loadPath;
    scriptLoadJob->submit();
    mLuaScriptLoadJobs.push_back(scriptLoadJob);
}

Texture2DAsset AssetManagerObj::get_texture_2d_asset(AUID auid)
{
    auto ite = mTexture2DAssets.find(auid);

    if (ite == mTexture2DAssets.end())
        return {};

    return ite->second;
}

MeshAsset AssetManagerObj::get_mesh_asset(AUID auid)
{
    auto ite = mMeshAssets.find(auid);

    if (ite == mMeshAssets.end())
        return {};

    return ite->second;
}

LuaScriptAsset AssetManagerObj::get_lua_script_asset(AUID auid)
{
    auto ite = mLuaScriptAssets.find(auid);

    if (ite == mLuaScriptAssets.end())
        return {};

    return ite->second;
}

AssetManager AssetManager::create(const std::filesystem::path& rootPath)
{
    AssetManagerObj* obj = heap_new<AssetManagerObj>(MEMORY_USAGE_ASSET, rootPath);

    return {obj};
}

void AssetManager::destroy(AssetManager manager)
{
    AssetManagerObj* obj = manager.unwrap();

    heap_delete<AssetManagerObj>(obj);
}

void AssetManager::begin_load_batch()
{
    mObj->begin_load_batch();
}

void AssetManager::end_load_batch()
{
    mObj->end_load_batch();
}

void AssetManager::load_mesh_asset(const fs::path& path, AUID auid)
{
    sLog.info("load_mesh_asset {}", path.string());

    mObj->load_mesh_asset(path, auid);
}

void AssetManager::load_texture_2d_asset(const fs::path& path, AUID auid)
{
    sLog.info("load_texture_2d_asset {}", path.string());

    mObj->load_texture_2d_asset(path, auid);
}

void AssetManager::load_lua_script_asset(const fs::path& path, AUID auid)
{
    sLog.info("load_lua_script_asset {}", path.string());

    mObj->load_lua_script_asset(path, auid);
}

MeshAsset AssetManager::get_mesh_asset(AUID auid)
{
    return mObj->get_mesh_asset(auid);
}

Texture2DAsset AssetManager::get_texture_2d_asset(AUID auid)
{
    return mObj->get_texture_2d_asset(auid);
}

LuaScriptAsset AssetManager::get_lua_script_asset(AUID auid)
{
    return mObj->get_lua_script_asset(auid);
}

} // namespace LD