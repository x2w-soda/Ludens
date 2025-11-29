#include <Ludens/Asset/AssetSchema.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Version.h>
#include <Ludens/Media/Format/TOML.h>
#include <Ludens/Profiler/Profiler.h>

namespace LD {

static void load_registry_from_schema(AssetRegistry registry, TOMLDocument doc);
static void load_registry_entries(AssetRegistry registry, TOMLDocument doc);
static void save_registry_to_schema(AssetRegistry registry, TOMLDocument doc);
static void save_registry_entries(AssetRegistry registry, TOMLDocument doc);

static void load_registry_from_schema(AssetRegistry registry, TOMLDocument doc)
{
    if (!registry || !doc)
        return;

    TOMLValue registryTOML = doc.get("ludens_assets");
    if (!registryTOML || registryTOML.get_type() != TOML_TYPE_TABLE)
        return;

    int32_t version;
    TOMLValue versionTOML = registryTOML["version_major"];
    if (!versionTOML || !versionTOML.get_i32(version) || version != LD_VERSION_MAJOR)
        return;

    versionTOML = registryTOML["version_minor"];
    if (!versionTOML || !versionTOML.get_i32(version) || version != LD_VERSION_MINOR)
        return;

    versionTOML = registryTOML["version_patch"];
    if (!versionTOML || !versionTOML.get_i32(version) || version != LD_VERSION_PATCH)
        return;

    uint32_t auidCounter = 1;
    TOMLValue counterTOML = registryTOML["auid_counter"];
    if (counterTOML)
        counterTOML.get_u32(auidCounter);

    registry.set_auid_counter(auidCounter);

    load_registry_entries(registry, doc);
}

static void load_registry_entries(AssetRegistry registry, TOMLDocument doc)
{
    for (int i = 0; i < (int)ASSET_TYPE_ENUM_COUNT; i++)
    {
        AssetType assetType = (AssetType)i;
        const char* typeCstr = get_asset_type_cstr(assetType);
        TOMLValue entryArrayTOML = doc.get(typeCstr);
        if (!entryArrayTOML || !entryArrayTOML.is_array_type())
            continue;

        int arraySize = entryArrayTOML.get_size();
        for (int j = 0; j < arraySize; j++)
        {
            TOMLValue entryTOML = entryArrayTOML[j];
            if (!entryTOML.is_table_type())
                continue;

            AssetEntry entry = {.type = assetType};
            TOMLValue uriTOML = entryTOML["uri"];
            TOMLValue nameTOML = entryTOML["name"];
            TOMLValue auidTOML = entryTOML["auid"];
            if (!uriTOML || !uriTOML.get_string(entry.uri) ||
                !nameTOML || !nameTOML.get_string(entry.name) ||
                !auidTOML || !auidTOML.get_u32(entry.id))
                continue;

            bool ok = registry.register_asset_with_id(entry);
            LD_ASSERT(ok); // TODO: invalid toml schema code path
        }
    }
}

static void save_registry_to_schema(AssetRegistry registry, TOMLDocument doc)
{
    TOMLValue registryTOML = doc.set("ludens_assets", TOML_TYPE_TABLE);
    registryTOML.set_key("version_major", TOML_TYPE_INT).set_i32(LD_VERSION_MAJOR);
    registryTOML.set_key("version_minor", TOML_TYPE_INT).set_i32(LD_VERSION_MINOR);
    registryTOML.set_key("version_patch", TOML_TYPE_INT).set_i32(LD_VERSION_PATCH);

    save_registry_entries(registry, doc);
}

static void save_registry_entries(AssetRegistry registry, TOMLDocument doc)
{
    for (int i = 0; i < (int)ASSET_TYPE_ENUM_COUNT; i++)
    {
        AssetType assetType = (AssetType)i;
        const char* typeCstr = get_asset_type_cstr(assetType);
        TOMLValue entryArrayTOML = doc.set(typeCstr, TOML_TYPE_ARRAY);

        std::vector<const AssetEntry*> entries;
        registry.find_assets_by_type(assetType, entries);

        for (const AssetEntry* entry : entries)
        {
            TOMLValue entryTOML = entryArrayTOML.append(TOML_TYPE_TABLE);
            entryTOML.set_key("uri", TOML_TYPE_STRING).set_string(entry->uri);
            entryTOML.set_key("name", TOML_TYPE_STRING).set_string(entry->name);
            entryTOML.set_key("auid", TOML_TYPE_INT).set_u32(entry->id);
        }
    }
}

//
// Public API
//

void AssetSchema::load_registry_from_file(AssetRegistry registry, const FS::Path& tomlPath)
{
    LD_PROFILE_SCOPE;

    TOMLDocument doc = TOMLDocument::create_from_file(tomlPath);
    load_registry_from_schema(registry, doc);
    TOMLDocument::destroy(doc);
}

bool AssetSchema::save_registry(AssetRegistry registry, const FS::Path& savePath, std::string& err)
{
    LD_PROFILE_SCOPE;

    TOMLDocument doc = TOMLDocument::create();
    save_registry_to_schema(registry, doc);

    std::string str;
    if (!doc.save_to_string(str))
    {
        TOMLDocument::destroy(doc);
        return false;
    }

    TOMLDocument::destroy(doc);
    return FS::write_file_and_swap_backup(savePath, str.size(), (const byte*)str.data(), err);
}

} // namespace LD