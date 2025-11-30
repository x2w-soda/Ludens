#pragma once

#include "AssetWatcher.h"
#include <Ludens/Asset/Asset.h>
#include <Ludens/Asset/AssetRegistry.h>
#include <Ludens/Asset/AssetType/AudioClipAsset.h>
#include <Ludens/Asset/AssetType/LuaScriptAsset.h>
#include <Ludens/Asset/AssetType/MeshAsset.h>
#include <Ludens/Asset/AssetType/Texture2DAsset.h>
#include <Ludens/DataRegistry/DataComponent.h>
#include <Ludens/Header/Hash.h>
#include <Ludens/Media/AudioData.h>
#include <Ludens/Media/Bitmap.h>
#include <Ludens/Media/Model.h>
#include <Ludens/System/Allocator.h>
#include <Ludens/System/FileSystem.h>
#include <Ludens/System/FileWatcher.h>
#include <filesystem>
#include <unordered_map>
#include <vector>

// first four bytes of any Ludens Asset file.
#define LD_ASSET_MAGIC "LDA."

namespace LD {

struct Serializer;
struct AssetManagerInfo;
struct AssetLoadJob;

/// @brief Asset manager implementation.
class AssetManagerObj
{
public:
    AssetManagerObj() = delete;
    AssetManagerObj(const AssetManagerInfo& info);
    AssetManagerObj(const AssetManagerObj&) = delete;
    ~AssetManagerObj();

    AssetManagerObj& operator=(const AssetManagerObj&) = delete;

    AssetObj* allocate_asset(AssetType type, AUID auid, const std::string& name);
    void free_asset(AssetObj* obj);

    AssetLoadJob* allocate_load_job(AssetType type, const FS::Path& loadPath, AssetObj* assetObj);
    void free_load_jobs();

    void poll();

    void begin_load_batch();
    void end_load_batch();
    void load_asset(AssetType type, AUID auid, const FS::Path& uri, const std::string& name);

    AUID get_id_from_name(const char* name, AssetType* outType);
    AssetHandle<AssetObj> get_asset(AUID auid);

    inline void find_assets_by_type(AssetType type, std::vector<const AssetEntry*>& entries)
    {
        mRegistry.find_assets_by_type(type, entries);
    }

    static void on_asset_modified(const FS::Path& path, AUID id, void* user);

private:
    std::unordered_map<AssetType, PoolAllocator> mAssetPA;
    std::unordered_map<AUID, AssetObj*> mAssets;
    std::unordered_map<Hash32, AUID> mNameToAsset;
    std::vector<AssetLoadJob*> mLoadJobs;
    PoolAllocator mLoadJobPA;
    AssetWatcher mWatcher;     /// optional asset file watcher
    AssetRegistry mRegistry;   /// bookkeeping for all assets in project
    const FS::Path mRootPath;  /// asset URIs are relative paths to root path
    bool mInLoadBatch = false; /// is within load batch scope
};

/// @brief Job context for loading an Asset.
/// @warning Address of this struct must not change since it is supplied as JobHeader::user,
///          that means worker threads will be accessing this struct.
struct AssetLoadJob
{
    FS::Path loadPath;                 /// path to .lda file on disk
    AssetHandle<AssetObj> assetHandle; /// base class handle
    JobHeader jobHeader;               /// submitted to the job system
};

/// @brief Audio clip asset implementation.
struct AudioClipAssetObj : AssetObj
{
    AudioData data;

    static void load(void* assetLoadJob);
    static void unload(AssetObj* base);
};

// NOTE: Placeholder Mesh asset implementation.
//       Need to figure out texture and material assets first.
struct MeshAssetObj : AssetObj
{
    ModelBinary* modelBinary;

    static void load(void* assetLoadJob);
    static void unload(AssetObj* base);
};

/// @brief Texture2D asset implementation.
struct Texture2DAssetObj : AssetObj
{
    RSamplerInfo samplerHint;
    TextureCompression compression;
    Bitmap bitmap;

    static void load(void* assetLoadJob);
    static void unload(AssetObj* base);
};

/// @brief Lua script asset implementation. Should contain enough information
///        to instantiate lua script instances.
struct LuaScriptAssetObj : AssetObj
{
    CUID duid;    /// component ID identifies script instance
    char* source; /// lua source code string

    static void load(void* assetLoadJob);
    static void unload(AssetObj* base);
};

/// @brief Polymorphic unload/cleanup for each asset type.
void asset_unload(AssetObj* base);

/// @brief Get 8 bytes of magic unique to each asset type.
/// @return Static C string of length 8.
const char* get_asset_type_magic_cstr(AssetType type);

/// @brief Write binary header for asset type.
/// @param serializer Serializer used to write the header.
/// @param type Asset type information to serialize.
void asset_header_write(Serializer& serializer, AssetType type);

/// @brief Attempts to read binary header from memory.
/// @param serializer Serializer used to read header.
/// @param outMajor Outputs the major framework version this asset is created with.
/// @param outMinor Outputs the minor framework version this asset is created with.
/// @param outPatch Outputs the patch framework version this asset is created with.
/// @param outType Outputs the asset type enum if recognized.
/// @return True if the header is recognized with this framework version, and the serializer cursor sits right after the header.
bool asset_header_read(Serializer& serializer, uint16_t& outMajor, uint16_t& outMinor, uint16_t& outPatch, AssetType& outType);

} // namespace LD