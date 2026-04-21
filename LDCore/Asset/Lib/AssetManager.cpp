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

#include "AssetLoadJob.h"
#include "AssetMeta.h"

namespace LD {

static Log sLog("AssetManager");
static AssetManagerObj* sAssetManager = nullptr;

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
    env = info.env;

    if (info.watchAssets)
    {
        AssetWatcherInfo watcherI{};
        watcherI.onAssetModified = &AssetManagerObj::on_asset_modified;
        watcherI.user = this;
        mWatcher.startup(watcherI);
    }

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

    for (auto pa : mAssetPA)
    {
        if (pa)
            PoolAllocator::destroy(pa);
    }

    if (mWatcher)
        mWatcher.cleanup();
}

AssetObj* AssetManagerObj::allocate_asset(AssetEntry entry)
{
    return allocate_asset(entry.get_type(), entry.get_id(), entry.get_name(), false);
}

AssetObj* AssetManagerObj::allocate_asset(AssetType type, SUID id, const std::string& path, bool isReserved)
{
    LD_ASSERT(id && !path.empty());

    if (!mAssetPA[(int)type])
    {
        PoolAllocatorInfo paI{};
        paI.usage = MEMORY_USAGE_ASSET;
        paI.blockSize = get_asset_byte_size(type);
        paI.pageSize = 16;
        paI.isMultiPage = true;
        mAssetPA[type] = PoolAllocator::create(paI);
    }

    AssetObj* obj = (AssetObj*)mAssetPA[type].allocate();
    sAssetMeta[(int)type].create(obj);

    obj->type = type;
    obj->id = id;
    obj->manager = this;
    obj->isReserved = isReserved;

    if (!obj->isReserved)
        register_asset(obj);

    return obj;
}

void AssetManagerObj::free_asset(AssetObj* obj)
{
    if (!obj)
        return;

    int type = (int)obj->type;

    LD_ASSERT(mAssetPA[type]);

    mAssets.erase(obj->id);

    sAssetMeta[type].destroy(obj);
    mAssetPA[type].free(obj);
}

void AssetManagerObj::register_asset(AssetObj* obj)
{
    LD_ASSERT(obj && obj->id);

    LD_ASSERT(!mAssets.contains(obj->id));
    mAssets[obj->id] = obj;
}

void AssetManagerObj::unregister_asset(AssetObj* obj)
{
    mAssets.erase(obj->id);
}

AssetLoadJob* AssetManagerObj::allocate_load_job(AssetEntry entry, AssetObj* assetObj, const FS::Path& dirPath)
{
    AssetLoadJob* job = (AssetLoadJob*)mLoadJobPA.allocate();
    new (job) AssetLoadJob();

    job->assetDirPath = dirPath;
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

bool AssetManagerObj::end_load_batch(Vector<AssetLoadStatus>& outErrors)
{
    LD_ASSERT(mInLoadBatch);

    mInLoadBatch = false;

    // TODO: wait for asset load jobs only
    JobSystem::get().wait_all();

    std::string err;
    outErrors.clear();

    for (AssetLoadJob* job : mLoadJobs)
    {
        if (!job->status)
            outErrors.push_back(job->status);

        job->~AssetLoadJob();
        mLoadJobPA.free(job);
    }

    mLoadJobs.clear();

    return outErrors.empty();
}

void AssetManagerObj::load_asset(AssetEntry entry)
{
    LD_ASSERT(mInLoadBatch);
    LD_ASSERT(!env.rootPath.empty());

    if (!entry)
        return;

    const AssetType type = entry.get_type();
    const FS::Path dirPath = env.get_asset_dir_path(entry.get_id());
    sLog.info("request asset [{}] {}", dirPath.string(), entry.get_name());

    /*
    TODO: per-type hooks after successful asset load
    if (mWatcher && type == ASSET_TYPE_LUA_SCRIPT)
    {
        FS::Path luaPath = FS::absolute(rootPath / FS::Path(entry.get_file_path("source")));
        mWatcher.add_watch(luaPath, entry.get_id());
    }
    */

    AssetObj* obj = allocate_asset(entry);
    AssetLoadJob* job = allocate_load_job(entry, obj, dirPath);
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
    if (!name || !env.registry)
        return 0;

    AssetEntry entry = env.registry.get_entry_by_name(name);

    if (!entry)
        return 0;

    if (outType)
        *outType = entry.get_type();

    return entry.get_id();
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
            scriptA.set_source(View((const char*)buf.data(), buf.size()));
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

std::string Asset::get_path()
{
    AssetRegistry AR = mObj->manager->env.registry;
    LD_ASSERT(AR);

    AssetEntry entry = AR.get_entry(mObj->id);
    LD_ASSERT(entry);

    return entry.get_path();
}

std::string Asset::get_name()
{
    AssetRegistry AR = mObj->manager->env.registry;
    LD_ASSERT(AR);

    AssetEntry entry = AR.get_entry(mObj->id);
    LD_ASSERT(entry);

    return entry.get_name();
}

AssetID Asset::get_id()
{
    return mObj->id;
}

AssetManager AssetManager::create(const AssetManagerInfo& info)
{
    LD_ASSERT(!sAssetManager);
    LD_ASSERT(!(info.env.registry && info.env.rootPath.empty()));

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
    return mObj->env.registry;
}

AssetRegistry AssetManager::swap_asset_registry(AssetRegistry registry, const FS::Path& rootPath)
{
    AssetRegistry oldReg = mObj->env.registry;

    mObj->unload_all_assets();
    mObj->env.registry = registry;
    mObj->env.rootPath = rootPath;

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
    mObj->load_asset(mObj->env.get_entry(id));
}

void AssetManager::begin_load_batch()
{
    mObj->begin_load_batch();
}

bool AssetManager::end_load_batch(Vector<AssetLoadStatus>& outErrors)
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

Asset AssetManager::alloc_reserved_asset(SUIDRegistry idReg, AssetType type, const std::string& path)
{
    // NOTE: this ID is not registered in AssetRegistry until successful resolve
    SUID reservedID = idReg.get_suid(SERIAL_TYPE_ASSET);

    AssetObj* obj = mObj->allocate_asset(type, reservedID, path, true);

    return Asset(obj);
}

void AssetManager::free_reserved_asset(SUIDRegistry idReg, Asset reservedAsset)
{
    AssetObj* obj = reservedAsset.unwrap();
    LD_ASSERT(obj && obj->isReserved); // this is a valid asset already

    idReg.free_suid(obj->id);

    mObj->free_asset(reservedAsset.unwrap());
}

AssetEntry AssetManager::resolve_asset(SUIDRegistry idReg, Asset reservedAsset)
{
    if (!reservedAsset || !mObj->env.registry)
        return {};

    LD_ASSERT(reservedAsset.unwrap()->isReserved);

    AssetEntry entry = mObj->env.registry.register_asset_with_id(idReg, reservedAsset.get_id(), reservedAsset.get_type(), reservedAsset.get_path());

    if (entry) // successfully resolved, the Asset handle could be used normally.
    {
        AssetObj* obj = reservedAsset.unwrap();
        obj->isReserved = false;
        mObj->register_asset(obj);
    }

    return entry;
}

} // namespace LD