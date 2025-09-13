#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/Asset/LuaScriptAsset.h>
#include <Ludens/Asset/MeshAsset.h>
#include <Ludens/Asset/TextureAsset.h>
#include <Ludens/DataRegistry/DataComponent.h>
#include <Ludens/Media/Bitmap.h>
#include <Ludens/Media/Model.h>
#include <Ludens/System/Allocator.h>
#include <filesystem>
#include <unordered_map>
#include <vector>

// note that this is an internal header,
// module public API should not do this.
namespace fs = std::filesystem;

namespace LD {

/// @brief Asset manager implementation.
class AssetManagerObj
{
public:
    AssetManagerObj() = delete;
    AssetManagerObj(const fs::path& rootPath);
    AssetManagerObj(const AssetManagerObj&) = delete;
    ~AssetManagerObj();

    AssetManagerObj& operator=(const AssetManagerObj&) = delete;

    AssetObj* allocate_asset(AssetType type, AUID auid, const std::string& name);
    void free_asset(AssetObj* obj);

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
    std::unordered_map<AUID, AssetObj*> mAssets;
    std::vector<struct MeshAssetLoadJob*> mMeshLoadJobs;
    std::vector<struct Texture2DAssetLoadJob*> mTexture2DLoadJobs;
    std::vector<struct LuaScriptAssetLoadJob*> mLuaScriptLoadJobs;
    const fs::path mRootPath;  /// asset URIs are relative paths to root path
    bool mInLoadBatch = false; /// is within load batch scope
};

// NOTE: Placeholder Mesh asset implementation.
//       Need to figure out texture and material assets first.
struct MeshAssetObj : AssetObj
{
    ModelBinary* modelBinary;

    static void unload(AssetObj* base);
};

/// @brief Texture2D asset implementation.
struct Texture2DAssetObj : AssetObj
{
    RSamplerInfo samplerHint;
    TextureCompression compression;
    Bitmap bitmap;

    static void unload(AssetObj* base);
};

/// @brief Lua script asset implementation. Should contain enough information
///        to instantiate lua script instances.
struct LuaScriptAssetObj : AssetObj
{
    CUID duid;    /// component ID identifies script instance
    char* source; /// lua source code string

    static void unload(AssetObj* base);
};

/// @brief Polymorphic unload/cleanup for each asset type.
extern void asset_unload(AssetObj* base);

} // namespace LD