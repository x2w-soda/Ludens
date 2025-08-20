#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/Asset/LuaScriptAsset.h>
#include <Ludens/Asset/MeshAsset.h>
#include <Ludens/Asset/TextureAsset.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/Media/Format/JSON.h>
#include <Ludens/RenderServer/RServer.h>
#include <filesystem>

namespace LD {

struct AssetManager : Handle<struct AssetManagerObj>
{
    static AssetManager create(const std::filesystem::path& rootPath);
    static void destroy(AssetManager manager);

    // NOTE: experimental
    void load_assets(JSONDocument assets);

    /// @brief Get mesh asset from ID.
    MeshAsset get_mesh_asset(AUID auid);

    /// @brief Get texture 2D asset from ID.
    Texture2DAsset get_texture_2d_asset(AUID auid);

    /// @brief Get Lua script asset from ID.
    LuaScriptAsset get_lua_script_asset(AUID auid);
};

} // namespace LD