#include "AssetObj.h"
#include <Ludens/Asset/AssetManager.h>
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

namespace LD {

static Log sLog("AssetManager");

// clang-format off
struct
{
    AssetType type;
    const char* typeMagic;          /// 8-byte magic embedded in binary header
    const char* typeName;           /// human readable name
    size_t size;                    /// object byte size
    void (*unload)(AssetObj* base); /// polymorphic unload
} sAssetTypeTable[] = {
    {ASSET_TYPE_AUDIO_CLIP, "AU_CLIP_", "AudioClip",  sizeof(AudioClipAssetObj), &AudioClipAssetObj::unload, },
    {ASSET_TYPE_MESH,       "MESH____", "Mesh",       sizeof(MeshAssetObj),      &MeshAssetObj::unload, },
    {ASSET_TYPE_TEXTURE_2D, "TX_2D___", "Texture2D",  sizeof(Texture2DAssetObj), &Texture2DAssetObj::unload, },
    {ASSET_TYPE_LUA_SCRIPT, "LUA_SCPT", "LuaScript",  sizeof(LuaScriptAssetObj), &LuaScriptAssetObj::unload, },
};
// clang-format on

static_assert(sizeof(sAssetTypeTable) / sizeof(*sAssetTypeTable) == ASSET_TYPE_ENUM_COUNT);
static_assert(LD::IsTrivial<AssetObj>);
static_assert(LD::IsTrivial<AudioClipAssetObj>);
static_assert(LD::IsTrivial<MeshAssetObj>);
static_assert(LD::IsTrivial<Texture2DAssetObj>);
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
}

AssetManagerObj::~AssetManagerObj()
{
    LD_PROFILE_SCOPE;

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

    for (auto ite : mAllocators)
        PoolAllocator::destroy(ite.second);

    if (mWatcher)
        mWatcher.cleanup();
}

AssetObj* AssetManagerObj::allocate_asset(AssetType type, AUID auid, const std::string& name)
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

    AssetObj* obj = (AssetObj*)mAllocators[type].allocate();
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
    LD_ASSERT(obj && mAllocators.contains(obj->type));

    mNameToAsset.erase(Hash32(obj->name));
    mAssets.erase(obj->auid);

    if (obj->name)
        heap_free((void*)obj->name);

    PoolAllocator pa = mAllocators[obj->type];
    pa.free(obj);
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

    mAudioClipLoadJobs.clear();
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

    for (AudioClipAssetLoadJob* job : mAudioClipLoadJobs)
        heap_delete<AudioClipAssetLoadJob>(job);

    for (MeshAssetLoadJob* job : mMeshLoadJobs)
        heap_delete<MeshAssetLoadJob>(job);

    for (Texture2DAssetLoadJob* job : mTexture2DLoadJobs)
        heap_delete<Texture2DAssetLoadJob>(job);

    for (LuaScriptAssetLoadJob* job : mLuaScriptLoadJobs)
        heap_delete<LuaScriptAssetLoadJob>(job);
}

void AssetManagerObj::load_audio_clip_asset(const FS::Path& path, AUID auid)
{
    LD_ASSERT(mInLoadBatch);

    auto obj = (AudioClipAssetObj*)allocate_asset(ASSET_TYPE_AUDIO_CLIP, auid, path.stem().string());

    FS::Path loadPath = mRootPath / path;

    auto audioClipLoadJob = heap_new<AudioClipAssetLoadJob>(MEMORY_USAGE_ASSET);
    audioClipLoadJob->asset = AudioClipAsset(obj);
    audioClipLoadJob->loadPath = loadPath;
    audioClipLoadJob->submit();
    mAudioClipLoadJobs.push_back(audioClipLoadJob);
}

void AssetManagerObj::load_mesh_asset(const FS::Path& path, AUID auid)
{
    LD_ASSERT(mInLoadBatch);

    auto obj = (MeshAssetObj*)allocate_asset(ASSET_TYPE_MESH, auid, path.stem().string());

    FS::Path loadPath = mRootPath / path;

    auto meshLoadJob = heap_new<MeshAssetLoadJob>(MEMORY_USAGE_ASSET);
    meshLoadJob->asset = MeshAsset(obj);
    meshLoadJob->loadPath = loadPath;
    meshLoadJob->submit();
    mMeshLoadJobs.push_back(meshLoadJob);
}

void AssetManagerObj::load_texture_2d_asset(const FS::Path& path, AUID auid)
{
    LD_ASSERT(mInLoadBatch);

    auto obj = (Texture2DAssetObj*)allocate_asset(ASSET_TYPE_TEXTURE_2D, auid, path.stem().string());

    FS::Path loadPath = mRootPath / path;

    auto textureLoadJob = heap_new<Texture2DAssetLoadJob>(MEMORY_USAGE_ASSET);
    textureLoadJob->asset = Texture2DAsset(obj);
    textureLoadJob->loadPath = loadPath;
    textureLoadJob->submit();
    mTexture2DLoadJobs.push_back(textureLoadJob);
}

void AssetManagerObj::load_lua_script_asset(const FS::Path& path, AUID auid)
{
    LD_ASSERT(mInLoadBatch);

    auto obj = (LuaScriptAssetObj*)allocate_asset(ASSET_TYPE_LUA_SCRIPT, auid, path.stem().string());

    FS::Path loadPath = mRootPath / path;

    if (mWatcher)
    {
        mWatcher.add_watch(loadPath, auid);
    }

    auto scriptLoadJob = heap_new<LuaScriptAssetLoadJob>(MEMORY_USAGE_ASSET);
    scriptLoadJob->asset = LuaScriptAsset(obj);
    scriptLoadJob->loadPath = loadPath;
    scriptLoadJob->submit();
    mLuaScriptLoadJobs.push_back(scriptLoadJob);
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

AudioClipAsset AssetManagerObj::get_audio_clip_asset(AUID auid)
{
    auto ite = mAssets.find(auid);

    if (ite == mAssets.end())
        return {};

    LD_ASSERT(ite->second->type == ASSET_TYPE_AUDIO_CLIP);
    return AudioClipAsset(ite->second);
}

Texture2DAsset AssetManagerObj::get_texture_2d_asset(AUID auid)
{
    auto ite = mAssets.find(auid);

    if (ite == mAssets.end())
        return {};

    LD_ASSERT(ite->second->type == ASSET_TYPE_TEXTURE_2D);
    return Texture2DAsset(ite->second);
}

MeshAsset AssetManagerObj::get_mesh_asset(AUID auid)
{
    auto ite = mAssets.find(auid);

    if (ite == mAssets.end())
        return {};

    LD_ASSERT(ite->second->type == ASSET_TYPE_MESH);
    return MeshAsset(ite->second);
}

LuaScriptAsset AssetManagerObj::get_lua_script_asset(AUID auid)
{
    auto ite = mAssets.find(auid);

    if (ite == mAssets.end())
        return {};

    LD_ASSERT(ite->second->type == ASSET_TYPE_LUA_SCRIPT);
    return LuaScriptAsset(ite->second);
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

void AssetManager::begin_load_batch()
{
    mObj->begin_load_batch();
}

void AssetManager::end_load_batch()
{
    mObj->end_load_batch();
}

void AssetManager::load_mesh_asset(const FS::Path& path, AUID auid)
{
    sLog.info("load_mesh_asset {}", path.string());

    mObj->load_mesh_asset(path, auid);
}

void AssetManager::load_audio_clip_asset(const FS::Path& path, AUID auid)
{
    sLog.info("load_audio_clip_asset {}", path.string());

    mObj->load_audio_clip_asset(path, auid);
}

void AssetManager::load_texture_2d_asset(const FS::Path& path, AUID auid)
{
    sLog.info("load_texture_2d_asset {}", path.string());

    mObj->load_texture_2d_asset(path, auid);
}

void AssetManager::load_lua_script_asset(const FS::Path& path, AUID auid)
{
    sLog.info("load_lua_script_asset {}", path.string());

    mObj->load_lua_script_asset(path, auid);
}

AUID AssetManager::get_id_from_name(const char* name, AssetType* outType)
{
    return mObj->get_id_from_name(name, outType);
}

AudioClipAsset AssetManager::get_audio_clip_asset(AUID auid)
{
    return mObj->get_audio_clip_asset(auid);
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

void asset_unload(AssetObj* base)
{
    LD_ASSERT(base);

    if (!sAssetTypeTable[base->type].unload)
        return;

    sAssetTypeTable[base->type].unload(base);
}

const char* get_asset_type_magic_cstr(AssetType type)
{
    return sAssetTypeTable[(int)type].typeMagic;
}

void asset_header_write(Serializer& serializer, AssetType type)
{
    serializer.write((const byte*)LD_ASSET_MAGIC, 4);
    serializer.write_u16(LD_VERSION_MAJOR);
    serializer.write_u16(LD_VERSION_MINOR);
    serializer.write_u16(LD_VERSION_PATCH);
    serializer.write((const byte*)get_asset_type_magic_cstr(type), 8);
}

bool asset_header_read(Serializer& serializer, uint16_t& outMajor, uint16_t& outMinor, uint16_t& outPatch, AssetType& outType)
{
    if (serializer.size() < 18)
        return false;

    char ldaMagic[4];
    serializer.read((byte*)ldaMagic, 4);

    if (strncmp(ldaMagic, LD_ASSET_MAGIC, 4))
        return false;

    serializer.read_u16(outMajor);
    serializer.read_u16(outMinor);
    serializer.read_u16(outPatch);

    char typeMagic[8];
    serializer.read((byte*)typeMagic, 8);

    // TODO: this can be hashed for O(1) lookup but we barely have any asset type variety right now.
    for (int type = 0; type < (int)ASSET_TYPE_ENUM_COUNT; type++)
    {
        const char* match = get_asset_type_magic_cstr((AssetType)type);

        if (!strncmp(typeMagic, match, 8))
        {
            outType = (AssetType)type;
            return true;
        }
    }

    // unrecognized asset type
    return false;
}

} // namespace LD