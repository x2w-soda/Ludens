#include <Ludens/Asset/AssetManager.h>
#include <Ludens/Asset/AssetSchema.h>
#include <Ludens/Asset/AssetType/MeshAsset.h>
#include <Ludens/Asset/AssetType/Texture2DAsset.h>
#include <Ludens/Header/Types.h>
#include <Ludens/Header/Version.h>
#include <Ludens/Log/Log.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/RenderBackend/RUtil.h>
#include <Ludens/RenderComponent/Layout/RMaterial.h>
#include <Ludens/RenderComponent/Layout/RMesh.h>
#include <Ludens/System/Allocator.h>

#include <string>
#include <unordered_map>

#include "AssetObj.h"
#include "AssetType/AudioClipAssetObj.h"
#include "AssetType/BlobAssetObj.h"
#include "AssetType/FontAssetObj.h"
#include "AssetType/LuaScriptAssetObj.h"
#include "AssetType/MeshAssetObj.h"
#include "AssetType/Texture2DAssetObj.h"
#include "AssetType/TextureCubeAssetObj.h"
#include "AssetType/UITemplateAssetObj.h"

namespace LD {

static Log sLog("AssetManager");

// clang-format off
struct
{
    AssetType type;
    const char* typeName;             /// human readable name
    size_t size;                      /// object byte size
    void (*load)(void* assetLoadJob); /// polymorphic load, note that this is executed on worker threads
    void (*unload)(AssetObj* base);   /// polymorphic unload
} sAssetTypeTable[] = {
    {ASSET_TYPE_BLOB,         "Blob",        sizeof(BlobAssetObj),        &BlobAssetObj::load,         &BlobAssetObj::unload, },
    {ASSET_TYPE_FONT,         "Font",        sizeof(FontAssetObj),        &FontAssetObj::load,         &FontAssetObj::unload, },
    {ASSET_TYPE_MESH,         "Mesh",        sizeof(MeshAssetObj),        &MeshAssetObj::load,         &MeshAssetObj::unload, },
    {ASSET_TYPE_UI_TEMPLATE,  "UITemplate",  sizeof(UITemplateAssetObj),  &UITemplateAssetObj::load,   &UITemplateAssetObj::unload, },
    {ASSET_TYPE_AUDIO_CLIP,   "AudioClip",   sizeof(AudioClipAssetObj),   &AudioClipAssetObj::load,    &AudioClipAssetObj::unload, },
    {ASSET_TYPE_TEXTURE_2D,   "Texture2D",   sizeof(Texture2DAssetObj),   &Texture2DAssetObj::load,    &Texture2DAssetObj::unload, },
    {ASSET_TYPE_TEXTURE_CUBE, "TextureCube", sizeof(TextureCubeAssetObj), &TextureCubeAssetObj::load,  &TextureCubeAssetObj::unload, },
    {ASSET_TYPE_LUA_SCRIPT,   "LuaScript",   sizeof(LuaScriptAssetObj),   &LuaScriptAssetObj::load,    &LuaScriptAssetObj::unload, },
};
// clang-format on

static_assert(sizeof(sAssetTypeTable) / sizeof(*sAssetTypeTable) == ASSET_TYPE_ENUM_COUNT);
static_assert(LD::IsTrivial<AssetObj>);
static_assert(LD::IsTrivial<BlobAssetObj>);
static_assert(LD::IsTrivial<FontAssetObj>);
static_assert(LD::IsTrivial<MeshAssetObj>);
static_assert(LD::IsTrivial<UITemplateAssetObj>);
static_assert(LD::IsTrivial<AudioClipAssetObj>);
static_assert(LD::IsTrivial<Texture2DAssetObj>);
static_assert(LD::IsTrivial<TextureCubeAssetObj>);
static_assert(LD::IsTrivial<LuaScriptAssetObj>);

size_t get_asset_byte_size(AssetType type)
{
    return sAssetTypeTable[(int)type].size;
}

const char* get_asset_type_cstr(AssetType type)
{
    return sAssetTypeTable[(int)type].typeName;
}

AssetManagerObj::AssetManagerObj(const AssetManagerInfo& info)
    : mRootPath(info.rootPath)
{
    if (info.watchAssets)
    {
        AssetWatcherInfo watcherI{};
        watcherI.onAssetModified = &AssetManagerObj::on_asset_modified;
        watcherI.user = this;
        mWatcher.startup(watcherI);
    }

    mRegistry = AssetRegistry::create();
    AssetSchema::load_registry_from_file(mRegistry, info.assetSchemaPath);

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

    std::vector<AssetObj*> assets;
    assets.reserve(mAssets.size());

    for (auto ite : mAssets)
        assets.push_back(ite.second);

    for (AssetObj* base : assets)
    {
        asset_unload(base);
        this->free_asset(base);
    }

    LD_ASSERT(mAssets.empty());

    for (auto ite : mAssetPA)
        PoolAllocator::destroy(ite.second);

    AssetRegistry::destroy(mRegistry);

    if (mWatcher)
        mWatcher.cleanup();
}

AssetObj* AssetManagerObj::allocate_asset(AssetType type, AUID auid, const std::string& name)
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
    obj->name = heap_strdup(name.c_str(), MEMORY_USAGE_ASSET);
    obj->auid = auid;
    obj->type = type;

    LD_ASSERT(auid && !mAssets.contains(obj->auid));
    mAssets[obj->auid] = obj;

    Hash32 nameHash(name.c_str());
    LD_ASSERT(!mNameToAsset.contains(nameHash));
    mNameToAsset[nameHash] = obj->auid;

    return obj;
}

void AssetManagerObj::free_asset(AssetObj* obj)
{
    LD_ASSERT(obj && mAssetPA.contains(obj->type));

    mNameToAsset.erase(Hash32(obj->name));
    mAssets.erase(obj->auid);

    if (obj->name)
        heap_free((void*)obj->name);

    PoolAllocator pa = mAssetPA[obj->type];
    pa.free(obj);
}

AssetLoadJob* AssetManagerObj::allocate_load_job(AssetType type, const FS::Path& loadPath, AssetObj* assetObj)
{
    AssetLoadJob* job = (AssetLoadJob*)mLoadJobPA.allocate();
    new (job) AssetLoadJob();

    job->loadPath = loadPath;
    job->assetHandle = {assetObj};
    job->jobHeader.fn = sAssetTypeTable[(int)type].load;
    job->jobHeader.type = (uint32_t)0; // TODO: job type for asset loading
    job->jobHeader.user = (void*)job;

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

void AssetManagerObj::end_load_batch()
{
    LD_ASSERT(mInLoadBatch);

    mInLoadBatch = false;

    // TODO: wait for asset load jobs only
    JobSystem::get().wait_all();

    free_load_jobs();
}

void AssetManagerObj::load_asset(AssetType type, AUID auid, const FS::Path& uri, const std::string& name)
{
    LD_ASSERT(mInLoadBatch);

    const FS::Path loadPath = FS::Path(mRootPath / uri).lexically_normal();
    sLog.info("load_asset {}", loadPath.string());

    if (mWatcher && type == ASSET_TYPE_LUA_SCRIPT)
    {
        mWatcher.add_watch(loadPath, auid);
    }

    AssetObj* obj = allocate_asset(type, auid, name);
    AssetLoadJob* job = allocate_load_job(type, loadPath, obj);
    mLoadJobs.push_back(job);

    // We need to guarantee that the address of the job header does not change.
    // method 1. allocations from a PoolAllocator do not migrate
    // method 2. individual heap_new / heap_malloc for each AssetLoadJob
    JobSystem::get().submit(&job->jobHeader, JOB_DISPATCH_STANDARD);
}

AUID AssetManagerObj::get_id_from_name(const char* name, AssetType* outType)
{
    if (!name)
        return 0;

    Hash32 nameHash(name);
    auto ite = mNameToAsset.find(nameHash);

    if (ite == mNameToAsset.end())
        return 0;

    AUID assetID = ite->second;
    LD_ASSERT(mAssets.contains(assetID));

    if (outType)
        *outType = mAssets[assetID]->type;

    return ite->second;
}

Asset AssetManagerObj::get_asset(AUID auid)
{
    auto ite = mAssets.find(auid);

    if (ite == mAssets.end())
        return {};

    return Asset(ite->second);
}

void AssetManagerObj::on_asset_modified(const FS::Path& path, AUID id, void* user)
{
    AssetManagerObj& self = *(AssetManagerObj*)user;

    auto ite = self.mAssets.find(id);
    if (ite == self.mAssets.end())
        return;

    // Experimental script reload. This only takes effect during the next Scene startup.

    AssetObj* assetObj = ite->second;
    if (assetObj && assetObj->type == ASSET_TYPE_LUA_SCRIPT)
    {
        LuaScriptAsset scriptA(assetObj);

        uint64_t fileSize = FS::get_file_size(path);
        Buffer buf;
        buf.resize(fileSize);
        if (fileSize > 0 && FS::read_file(path, fileSize, buf.data()))
        {
            scriptA.set_source((const char*)buf.data(), (size_t)buf.size());
        }
    }
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

AUID Asset::get_auid()
{
    return mObj->auid;
}

AssetManager AssetManager::create(const AssetManagerInfo& info)
{
    AssetManagerObj* obj = heap_new<AssetManagerObj>(MEMORY_USAGE_ASSET, info);

    return {obj};
}

void AssetManager::destroy(AssetManager manager)
{
    AssetManagerObj* obj = manager.unwrap();

    heap_delete<AssetManagerObj>(obj);
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

        std::vector<const AssetEntry*> entries;
        mObj->find_assets_by_type(type, entries);

        for (const AssetEntry* entry : entries)
        {
            mObj->load_asset(entry->type, entry->id, entry->uri, entry->name);
        }
    }
}

void AssetManager::load_asset(AssetType type, AUID auid, const FS::Path& path, const std::string& name)
{
    mObj->load_asset(type, auid, path, name);
}

void AssetManager::begin_load_batch()
{
    mObj->begin_load_batch();
}

void AssetManager::end_load_batch()
{
    mObj->end_load_batch();
}

AUID AssetManager::get_id_from_name(const char* name, AssetType* outType)
{
    return mObj->get_id_from_name(name, outType);
}

Asset AssetManager::get_asset(AUID auid)
{
    return mObj->get_asset(auid);
}

Asset AssetManager::get_asset(AUID auid, AssetType type)
{
    Asset asset = mObj->get_asset(auid);

    if (!asset || asset.get_type() != type)
        return {};

    return asset;
}

Asset AssetManager::get_asset(const char* name, AssetType type)
{
    AssetType assetType;
    AUID assetID = mObj->get_id_from_name(name, &assetType);
    Asset asset = mObj->get_asset(assetID);

    if (!asset || asset.get_type() != type)
        return {};

    return asset;
}

void asset_unload(AssetObj* base)
{
    LD_ASSERT(base);

    if (!sAssetTypeTable[base->type].unload)
        return;

    sAssetTypeTable[base->type].unload(base);
}

const char* get_asset_type_name_cstr(AssetType type)
{
    return sAssetTypeTable[(int)type].typeName;
}

void asset_header_write(Serializer& serial, AssetType type)
{
    serial.write((const byte*)LD_ASSET_MAGIC, 4);
    serial.write_u16(LD_VERSION_MAJOR);
    serial.write_u16(LD_VERSION_MINOR);
    serial.write_u16(LD_VERSION_PATCH);

    const char* typeName = get_asset_type_name_cstr(type);
    size_t len = strlen(typeName);

    serial.write_u16((uint16_t)len);
    serial.write((const byte*)typeName, len);
}

bool asset_header_read(Deserializer& serial, uint16_t& outMajor, uint16_t& outMinor, uint16_t& outPatch, AssetType& outType)
{
    if (serial.size() < 18)
        return false;

    char ldaMagic[4];
    serial.read((byte*)ldaMagic, 4);

    if (strncmp(ldaMagic, LD_ASSET_MAGIC, 4))
        return false;

    serial.read_u16(outMajor);
    serial.read_u16(outMinor);
    serial.read_u16(outPatch);

    uint16_t len;
    serial.read_u16(len);

    std::string typeName;
    typeName.resize(len);
    serial.read((byte*)typeName.data(), typeName.size());

    // TODO: this can be hashed for O(1) lookup but we barely have any asset type variety right now.
    for (int type = 0; type < (int)ASSET_TYPE_ENUM_COUNT; type++)
    {
        const char* match = get_asset_type_name_cstr((AssetType)type);

        if (typeName == match)
        {
            outType = (AssetType)type;
            return true;
        }
    }

    // unrecognized asset type
    return false;
}

} // namespace LD