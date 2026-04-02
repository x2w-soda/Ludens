#include <Ludens/Asset/AssetManager.h>
#include <Ludens/Asset/AssetSchema.h>
#include <Ludens/Asset/AssetType/MeshAsset.h>
#include <Ludens/Asset/AssetType/Texture2DAsset.h>
#include <Ludens/DSA/HashMap.h>
#include <Ludens/Header/Types.h>
#include <Ludens/Log/Log.h>
#include <Ludens/Memory/Allocator.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/RenderBackend/RUtil.h>
#include <Ludens/RenderComponent/Layout/RMaterial.h>
#include <Ludens/RenderComponent/Layout/RMesh.h>

#include <string>

#include "AssetMeta.h"

namespace LD {

static Log sLog("AssetManager");
static AssetManagerObj* sAssetManager = nullptr;

static_assert(LD::IsTrivial<AssetObj>);

size_t get_asset_byte_size(AssetType type)
{
    return sAssetMeta[(int)type].size;
}

const char* get_asset_type_cstr(AssetType type)
{
    return sAssetMeta[(int)type].typeName;
}

bool get_cstr_asset_type(const char* cstr, AssetType& outType)
{
    if (!cstr)
        return false;

    std::string str(cstr);

    for (int i = 0; i < (int)ASSET_TYPE_ENUM_COUNT; i++)
    {
        if (str == sAssetMeta[i].typeName)
        {
            outType = sAssetMeta[i].type;
            return true;
        }
    }

    return false;
}

AssetManagerObj::AssetManagerObj(const AssetManagerInfo& info)
{
    registry = info.registry;
    rootPath = info.rootPath;

    if (info.watchAssets)
    {
        AssetWatcherInfo watcherI{};
        watcherI.onAssetModified = &AssetManagerObj::on_asset_modified;
        watcherI.user = this;
        mWatcher.startup(watcherI);
    }

    registry = info.registry; // nullable.

    PoolAllocatorInfo paI{};
    paI.blockSize = sizeof(AssetLoadJob);
    paI.isMultiPage = true;
    paI.pageSize = 32;
    paI.usage = MEMORY_USAGE_ASSET;
    mLoadJobPA = PoolAllocator::create(paI);
}

AssetManagerObj::~AssetManagerObj()
{
    LD_PROFILE_SCOPE;

    free_load_jobs();
    PoolAllocator::destroy(mLoadJobPA);

    unload_all_assets();

    for (auto it : mAssetPA)
        PoolAllocator::destroy(it.second);

    if (mWatcher)
        mWatcher.cleanup();
}

AssetObj* AssetManagerObj::allocate_asset(AssetEntry entry)
{
    std::string name = entry.get_name();

    return allocate_asset(entry.get_type(), entry.get_id(), name.c_str());
}

AssetObj* AssetManagerObj::allocate_asset(AssetType type, SUID id, const char* name)
{
    size_t assetByteSize = get_asset_byte_size(type);

    if (!mAssetPA.contains(type))
    {
        PoolAllocatorInfo paI{};
        paI.usage = MEMORY_USAGE_ASSET;
        paI.blockSize = assetByteSize;
        paI.pageSize = 16;
        paI.isMultiPage = true;
        mAssetPA[type] = PoolAllocator::create(paI);
    }

    AssetObj* obj = (AssetObj*)mAssetPA[type].allocate();
    memset(obj, 0, assetByteSize);

    obj->manager = this;
    obj->name = name ? heap_strdup(name, MEMORY_USAGE_ASSET) : nullptr;
    obj->id = id;
    obj->type = type;

    if (obj->id)
    {
        LD_ASSERT(!mAssets.contains(obj->id));
        mAssets[obj->id] = obj;
    }

    if (obj->name)
    {
        LD_ASSERT(!mNameToAsset.contains(obj->name));
        mNameToAsset[obj->name] = obj->id;
    }

    return obj;
}

void AssetManagerObj::free_asset(AssetObj* obj)
{
    LD_ASSERT(obj && mAssetPA.contains(obj->type));

    mNameToAsset.erase(obj->name);
    mAssets.erase(obj->id);

    if (obj->name)
    {
        heap_free((void*)obj->name);
        obj->name = nullptr;
    }

    PoolAllocator pa = mAssetPA[obj->type];
    pa.free(obj);
}

AssetLoadJob* AssetManagerObj::allocate_load_job(AssetEntry entry, AssetObj* assetObj)
{
    AssetLoadJob* job = (AssetLoadJob*)mLoadJobPA.allocate();
    new (job) AssetLoadJob();

    job->rootPath = rootPath;
    job->loadPath = FS::absolute(rootPath / entry.get_main_path());
    job->assetHandle = {assetObj};
    job->assetEntry = entry;
    job->jobHeader.onExecute = sAssetMeta[(int)entry.get_type()].load;
    job->jobHeader.onComplete = &AssetManagerObj::on_asset_load_complete;
    job->jobHeader.type = (uint32_t)0; // TODO: job type for asset loading
    job->jobHeader.user = (void*)job;
    job->jobInProgress.store(true, std::memory_order_release); // NOTE: job is already considered in-progress before its submission
    job->jobProgress.store(0.0f, std::memory_order_release);

    return job;
}

void AssetManagerObj::free_load_jobs()
{
    for (AssetLoadJob* job : mLoadJobs)
    {
        job->~AssetLoadJob();
        mLoadJobPA.free(job);
    }

    mLoadJobs.clear();
}

bool AssetManagerObj::has_load_job()
{
    for (AssetLoadJob* job : mLoadJobs)
    {
        if (job->jobInProgress.load(std::memory_order_acquire))
            return true;
    }

    return false;
}

void AssetManagerObj::poll()
{
    if (mWatcher)
        mWatcher.poll();
}

void AssetManagerObj::begin_load_batch()
{
    LD_ASSERT(!mInLoadBatch);

    mInLoadBatch = true;

    // just sanity checking
    free_load_jobs();
}

bool AssetManagerObj::end_load_batch(Vector<std::string>& outErrors)
{
    LD_ASSERT(mInLoadBatch);

    mInLoadBatch = false;

    // TODO: wait for asset load jobs only
    JobSystem::get().wait_all();

    std::string err;
    outErrors.clear();

    for (AssetLoadJob* job : mLoadJobs)
    {
        if (job->diagnostics.get_error(err))
            outErrors.push_back(err);

        job->~AssetLoadJob();
        mLoadJobPA.free(job);
    }

    mLoadJobs.clear();

    return outErrors.empty();
}

void AssetManagerObj::load_asset(AssetEntry entry)
{
    LD_ASSERT(mInLoadBatch);
    LD_ASSERT(!rootPath.empty());

    if (!entry)
        return;

    const AssetType type = entry.get_type();
    const FS::Path loadPath = FS::absolute(rootPath / FS::Path(entry.get_main_path()));
    sLog.info("load_asset {}", loadPath.string());

    if (mWatcher && type == ASSET_TYPE_LUA_SCRIPT)
    {
        FS::Path luaPath = FS::absolute(rootPath / FS::Path(entry.get_path("source")));
        mWatcher.add_watch(luaPath, entry.get_id());
    }

    AssetObj* obj = allocate_asset(entry);
    AssetLoadJob* job = allocate_load_job(entry, obj);
    mLoadJobs.push_back(job);

    // We need to guarantee that the address of the job header does not change.
    // method 1. allocations from a PoolAllocator do not migrate
    // method 2. individual heap_new / heap_malloc for each AssetLoadJob
    JobSystem::get().submit(&job->jobHeader, JOB_DISPATCH_STANDARD);
}

void AssetManagerObj::unload_all_assets()
{
    std::vector<AssetObj*> assets;
    assets.reserve(mAssets.size());

    for (auto it : mAssets)
        assets.push_back(it.second);

    for (AssetObj* base : assets)
    {
        asset_unload(base);
        free_asset(base);
    }

    LD_ASSERT(mAssets.empty());
}

SUID AssetManagerObj::get_id_from_name(const char* name, AssetType* outType)
{
    if (!name)
        return 0;

    auto it = mNameToAsset.find(name);

    if (it == mNameToAsset.end())
        return 0;

    SUID assetID = it->second;
    LD_ASSERT(mAssets.contains(assetID));

    if (outType)
        *outType = mAssets[assetID]->type;

    return it->second;
}

Asset AssetManagerObj::get_asset(SUID id)
{
    auto it = mAssets.find(id);

    if (it == mAssets.end())
        return {};

    return Asset(it->second);
}

void AssetManagerObj::on_asset_modified(const FS::Path& path, SUID id, void* user)
{
    AssetManagerObj& self = *(AssetManagerObj*)user;

    auto it = self.mAssets.find(id);
    if (it == self.mAssets.end())
        return;

    // Experimental script reload. This only takes effect during the next Scene startup.

    AssetObj* assetObj = it->second;
    if (assetObj && assetObj->type == ASSET_TYPE_LUA_SCRIPT)
    {
        LuaScriptAsset scriptA(assetObj);

        std::string err;
        Vector<byte> buf;
        if (FS::read_file_to_vector(path, buf, err))
        {
            scriptA.set_source((const char*)buf.data(), (size_t)buf.size());
        }
    }
}

void AssetManagerObj::on_asset_load_complete(void* user)
{
    auto* job = (AssetLoadJob*)user;

    job->jobInProgress.store(false, std::memory_order_release);
    job->jobProgress.store(1.0f, std::memory_order_release);
}

//
// Public API
//

AssetType Asset::get_type()
{
    return mObj->type;
}

const char* Asset::get_name()
{
    return mObj->name;
}

AssetID Asset::get_id()
{
    return mObj->id;
}

AssetManager AssetManager::create(const AssetManagerInfo& info)
{
    LD_ASSERT(!sAssetManager);
    LD_ASSERT(!(info.registry && info.rootPath.empty()));

    sAssetManager = heap_new<AssetManagerObj>(MEMORY_USAGE_ASSET, info);

    return AssetManager(sAssetManager);
}

void AssetManager::destroy()
{
    LD_ASSERT(sAssetManager);

    heap_delete<AssetManagerObj>(sAssetManager);
    sAssetManager = nullptr;
}

AssetManager AssetManager::get()
{
    return AssetManager(sAssetManager);
}

AssetRegistry AssetManager::get_asset_registry()
{
    return mObj->registry;
}

AssetRegistry AssetManager::swap_asset_registry(AssetRegistry registry, const FS::Path& rootPath)
{
    AssetRegistry oldReg = mObj->registry;

    mObj->unload_all_assets();
    mObj->registry = registry;
    mObj->rootPath = rootPath;

    // Manager does not own the Registry,
    // caller need to destroy the registry later.
    return oldReg;
}

void AssetManager::update()
{
    mObj->poll();
}

void AssetManager::load_all_assets()
{
    LD_PROFILE_SCOPE;

    for (int i = 0; i < (int)ASSET_TYPE_ENUM_COUNT; i++)
    {
        AssetType type = (AssetType)i;

        Vector<AssetEntry> entries;
        mObj->get_entries_by_type(entries, type);

        for (AssetEntry entry : entries)
        {
            mObj->load_asset(entry);
        }
    }
}

void AssetManager::load_asset(AssetID id)
{
    mObj->load_asset(mObj->get_entry(id));
}

void AssetManager::begin_load_batch()
{
    mObj->begin_load_batch();
}

bool AssetManager::end_load_batch(Vector<std::string>& outErrors)
{
    return mObj->end_load_batch(outErrors);
}

bool AssetManager::has_load_job()
{
    return mObj->has_load_job();
}

SUID AssetManager::get_id_from_name(const char* name, AssetType* outType)
{
    return mObj->get_id_from_name(name, outType);
}

Asset AssetManager::get_asset(AssetID id)
{
    if (!id)
        return {};

    return mObj->get_asset(id);
}

Asset AssetManager::get_asset(AssetID id, AssetType type)
{
    Asset asset = mObj->get_asset(id);

    if (!asset || asset.get_type() != type)
        return {};

    return asset;
}

Asset AssetManager::get_asset(const char* name, AssetType type)
{
    AssetType assetType;
    SUID assetID = mObj->get_id_from_name(name, &assetType);
    Asset asset = mObj->get_asset(assetID);

    if (!asset || asset.get_type() != type)
        return {};

    return asset;
}

Asset AssetManager::reserve_asset(AssetType type)
{
    AssetObj* obj = mObj->allocate_asset(type, SUID(0), nullptr);

    return Asset(obj);
}

AssetEntry AssetManager::resolve_asset(Asset asset, const std::string& uri)
{
    if (!asset || !mObj->registry)
        return {};

    return mObj->registry.register_asset(asset.get_type(), uri);
}

} // namespace LD