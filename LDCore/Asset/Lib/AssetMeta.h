#pragma once

#include <Ludens/Asset/AssetManager.h>
#include <Ludens/Asset/AssetType/AudioClipAsset.h>
#include <Ludens/Asset/AssetType/LuaScriptAsset.h>
#include <Ludens/Asset/AssetType/MeshAsset.h>
#include <Ludens/Asset/AssetType/Texture2DAsset.h>
#include <Ludens/Asset/AssetType/TextureCubeAsset.h>
#include <Ludens/Asset/Template/UITemplate.h>
#include <Ludens/DSA/Array.h>
#include <Ludens/DSA/HashMap.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/DataRegistry/DataRegistry.h>
#include <Ludens/Header/Hash.h>
#include <Ludens/Media/AudioData.h>
#include <Ludens/Media/Bitmap.h>
#include <Ludens/Media/Font.h>
#include <Ludens/Media/Model.h>
#include <Ludens/Memory/Allocator.h>
#include <Ludens/System/FileSystem.h>
#include <Ludens/System/FileWatcher.h>

#include <cstdint>
#include <string>

#include "AssetWatcher.h"

// first four bytes of any Ludens Asset file.
#define LD_ASSET_MAGIC "LDA."

namespace LD {

struct Serializer;
struct AssetManagerInfo;
struct AssetLoadJob;

struct AssetMeta
{
    AssetType type;
    const char* typeName;             /// human readable name
    size_t size;                      /// object byte size
    void (*create)(AssetObj* base);   /// probably placement new
    void (*destroy)(AssetObj* base);  /// probably placement delete
    void (*load)(void* assetLoadJob); /// polymorphic load, note that this is executed on worker threads
    void (*unload)(AssetObj* base);   /// polymorphic unload
};

extern AssetMeta sAssetMeta[];

/// @brief Asset manager implementation.
class AssetManagerObj
{
public:
    AssetManagerObj() = delete;
    AssetManagerObj(const AssetManagerInfo& info);
    AssetManagerObj(const AssetManagerObj&) = delete;
    ~AssetManagerObj();

    AssetManagerObj& operator=(const AssetManagerObj&) = delete;

    AssetObj* allocate_asset(AssetEntry entry);
    AssetObj* allocate_asset(AssetType type, SUID id, bool isReserved);
    void free_asset(AssetObj* obj);
    void register_asset(AssetObj* obj);
    void unregister_asset(AssetObj* obj);

    AssetLoadJob* allocate_load_job(AssetEntry entry, AssetObj* assetObj, const FS::Path& loadPath);
    void free_load_jobs();
    bool has_load_job();

    void poll();

    void begin_load_batch();
    bool end_load_batch(Vector<AssetLoadStatus>& outErrors);
    void load_asset(AssetEntry entry);
    void unload_all_assets();

    SUID get_id_from_name(const char* name, AssetType* outType);
    Asset get_asset(SUID id);

    inline void get_entries_by_type(Vector<AssetEntry>& entries, AssetType type)
    {
        entries.clear();

        if (env.registry)
            env.registry.get_entries_by_type(entries, type);
    }

    static void on_asset_modified(const FS::Path& path, SUID id, void* user);
    static void on_asset_load_complete(void*);

public:
    AssetManagerEnv env = {};

private:
    Array<PoolAllocator, ASSET_TYPE_ENUM_COUNT> mAssetPA = {};
    HashMap<SUID, AssetObj*> mAssets;
    Vector<AssetLoadJob*> mLoadJobs;
    PoolAllocator mLoadJobPA = {}; /// provides address stability for each load job
    AssetWatcher mWatcher;         /// optional asset file watcher
    bool mInLoadBatch = false;     /// is within load batch scope
};

/// @brief Polymorphic unload/cleanup for each asset type.
void asset_unload(AssetObj* base);

/// @return Static C string of asset type enum.
const char* get_asset_type_name_cstr(AssetType type);

} // namespace LD