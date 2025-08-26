#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/Asset/LuaScriptAsset.h>
#include <Ludens/Asset/MeshAsset.h>
#include <Ludens/Asset/TextureAsset.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/RenderServer/RServer.h>
#include <filesystem>

namespace LD {

struct AssetManager : Handle<struct AssetManagerObj>
{
    static AssetManager create(const std::filesystem::path& rootPath);
    static void destroy(AssetManager manager);

    /// @brief Begin asset load batch.
    void begin_load_batch();

    /// @brief End asset load batch.
    void end_load_batch();

    /// @brief Append mesh asset load job to current batch.
    void load_mesh_asset(const std::filesystem::path& path, AUID auid);

    /// @brief Append texture 2D asset load job to current batch.
    void load_texture_2d_asset(const std::filesystem::path& path, AUID auid);

    /// @brief Append lua script asset load job to current batch.
    void load_lua_script_asset(const std::filesystem::path& path, AUID auid);

    /// @brief Get mesh asset from ID.
    MeshAsset get_mesh_asset(AUID auid);

    /// @brief Get texture 2D asset from ID.
    Texture2DAsset get_texture_2d_asset(AUID auid);

    /// @brief Get Lua script asset from ID.
    LuaScriptAsset get_lua_script_asset(AUID auid);
};

} // namespace LD