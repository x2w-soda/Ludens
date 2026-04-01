#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/Asset/AssetRegistry.h>
#include <Ludens/Asset/AssetType/AudioClipAsset.h>
#include <Ludens/Asset/AssetType/LuaScriptAsset.h>
#include <Ludens/Asset/AssetType/MeshAsset.h>
#include <Ludens/Asset/AssetType/Texture2DAsset.h>
#include <Ludens/Asset/AssetType/TextureCubeAsset.h>
#include <Ludens/Asset/Template/UITemplate.h>
#include <Ludens/DSA/Diagnostics.h>
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

#include <atomic>
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
    void free_asset(AssetObj* obj);

    AssetLoadJob* allocate_load_job(AssetEntry entry, AssetObj* assetObj);
    void free_load_jobs();
    bool has_load_job();

    void poll();

    void begin_load_batch();
    bool end_load_batch(Vector<std::string>& outErrors);
    void load_asset(AssetEntry entry);
    void unload_all_assets();

    SUID get_id_from_name(const char* name, AssetType* outType);
    Asset get_asset(SUID id);

    inline AssetEntry get_entry(SUID id)
    {
        return registry ? registry.get_entry(id) : AssetEntry();
    }

    inline void get_entries_by_type(Vector<AssetEntry>& entries, AssetType type)
    {
        entries.clear();

        if (registry)
            registry.get_entries_by_type(entries, type);
    }

    static void on_asset_modified(const FS::Path& path, SUID id, void* user);
    static void on_asset_load_complete(void*);

public:
    AssetRegistry registry = {}; /// bookkeeping for all assets in project, not owned by manager
    FS::Path rootPath = {};      /// asset file paths are relative to project root

private:
    HashMap<AssetType, PoolAllocator> mAssetPA;
    HashMap<SUID, AssetObj*> mAssets;
    HashMap<std::string, SUID> mNameToAsset;
    Vector<AssetLoadJob*> mLoadJobs;
    PoolAllocator mLoadJobPA = {}; /// provides address stability for each load job
    AssetWatcher mWatcher;         /// optional asset file watcher
    bool mInLoadBatch = false;     /// is within load batch scope
};

/// @brief Job context for loading an Asset.
/// @warning Address of this struct must not change since it is supplied as JobHeader::user,
///          that means worker threads will be accessing this struct.
struct AssetLoadJob
{
    JobHeader jobHeader;            /// submitted to the job system
    Asset assetHandle;              /// accessed by job thread, dst class handle
    AssetEntry assetEntry;          /// accessed by job thread, src asset to load
    Diagnostics diagnostics;        /// accessed by job thread throughout load job
    FS::Path loadPath;              /// concatenates root direcotry path and uri path
    FS::Path rootPath;              /// project root directory
    std::atomic_bool jobInProgress; /// read by main thread
    std::atomic<float> jobProgress; /// read by main thread, normalized job progress estimate
};

/// @brief Polymorphic unload/cleanup for each asset type.
void asset_unload(AssetObj* base);

/// @return Static C string of asset type enum.
const char* get_asset_type_name_cstr(AssetType type);

} // namespace LD