#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/Asset/AssetType/AudioClipAsset.h>
#include <Ludens/Asset/AssetType/LuaScriptAsset.h>
#include <Ludens/Asset/AssetType/MeshAsset.h>
#include <Ludens/Asset/AssetType/Texture2DAsset.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/RenderServer/RServer.h>
#include <Ludens/System/FileSystem.h>

namespace LD {

struct AssetManagerInfo
{
    FS::Path rootPath; // project root directory path
    bool watchAssets;  // whether to watch asset files in project
};

struct AssetManager : Handle<struct AssetManagerObj>
{
    /// @brief Create asset manager.
    static AssetManager create(const AssetManagerInfo& info);

    /// @brief Destroy asset manager.
    static void destroy(AssetManager manager);

    /// @brief If file watching is enabled, this polls for any changes.
    void update();

    /// @brief Begin asset load batch.
    void begin_load_batch();

    /// @brief End asset load batch.
    void end_load_batch();

    /// @brief Append mesh asset load job to current batch.
    void load_mesh_asset(const FS::Path& path, AUID auid);

    /// @brief Append audio clip asset load job to current batch.
    void load_audio_clip_asset(const FS::Path& path, AUID auid);

    /// @brief Append texture 2D asset load job to current batch.
    void load_texture_2d_asset(const FS::Path& path, AUID auid);

    /// @brief Append lua script asset load job to current batch.
    void load_lua_script_asset(const FS::Path& path, AUID auid);

    /// @brief Get asset ID from name
    AUID get_id_from_name(const char* name, AssetType* outType);

    /// @brief Get audio clip asset from ID
    AudioClipAsset get_audio_clip_asset(AUID auid);

    /// @brief Get mesh asset from ID.
    MeshAsset get_mesh_asset(AUID auid);

    /// @brief Get texture 2D asset from ID.
    Texture2DAsset get_texture_2d_asset(AUID auid);

    /// @brief Get Lua script asset from ID.
    LuaScriptAsset get_lua_script_asset(AUID auid);
};

} // namespace LD