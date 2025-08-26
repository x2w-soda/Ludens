#include <Ludens/Asset/AssetSchema.h>
#include <Ludens/Profiler/Profiler.h>

namespace LD {

void AssetSchema::load_assets(AssetManager manager, TOMLDocument doc)
{
    LD_PROFILE_SCOPE;

    TOMLValue assetsTOML = doc.get("ludens_assets");
    if (!assetsTOML || !assetsTOML.is_table_type())
        return;

    int32_t version;
    TOMLValue versionTOML = assetsTOML["version"];
    if (!versionTOML || !versionTOML.is_i32(version) || version != 0)
        return;

    manager.begin_load_batch();
    {
        TOMLValue meshesTOML = doc.get("Mesh");
        if (!meshesTOML || !meshesTOML.is_array_type())
            return;

        int count = meshesTOML.get_size();
        for (int i = 0; i < count; i++)
        {
            TOMLValue meshTOML = meshesTOML[i];
            TOMLValue auidTOML = meshTOML["auid"];
            TOMLValue uriTOML = meshTOML["uri"];
            int64_t auid;
            std::string uri;
            if (auidTOML && auidTOML.is_i64(auid) && uriTOML && uriTOML.is_string(uri))
            {
                manager.load_mesh_asset(std::filesystem::path(uri), (AUID)auid);
            }
        }

        TOMLValue texture2DsTOML = doc.get("Texture2D");
        if (!texture2DsTOML || !texture2DsTOML.is_array_type())
            return;

        count = texture2DsTOML.get_size();
        for (int i = 0; i < count; i++)
        {
            TOMLValue texture2DTOML = texture2DsTOML[i];
            TOMLValue auidTOML = texture2DTOML["auid"];
            TOMLValue uriTOML = texture2DTOML["uri"];
            int64_t auid;
            std::string uri;
            if (auidTOML && auidTOML.is_i64(auid) && uriTOML && uriTOML.is_string(uri))
            {
                manager.load_texture_2d_asset(std::filesystem::path(uri), (AUID)auid);
            }
        }

        TOMLValue luaScriptsTOML = doc.get("LuaScript");
        if (!luaScriptsTOML || !luaScriptsTOML.is_array_type())
            return;

        count = luaScriptsTOML.get_size();
        for (int i = 0; i < count; i++)
        {
            TOMLValue luaScriptTOML = luaScriptsTOML[i];
            TOMLValue auidTOML = luaScriptTOML["auid"];
            TOMLValue uriTOML = luaScriptTOML["uri"];
            int64_t auid;
            std::string uri;
            if (auidTOML && auidTOML.is_i64(auid) && uriTOML && uriTOML.is_string(uri))
            {
                manager.load_lua_script_asset(std::filesystem::path(uri), (AUID)auid);
            }
        }
    }
    manager.end_load_batch();
}

} // namespace LD