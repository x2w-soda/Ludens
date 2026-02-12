#include <Ludens/Asset/AssetSchema.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Version.h>
#include <Ludens/Media/Format/TOML.h>
#include <Ludens/Profiler/Profiler.h>

#include <format>

#include "AssetSchemaKeys.h"

namespace LD {

/// @brief Loads AssetRegistry from TOML
class AssetSchemaLoader
{
public:
    AssetSchemaLoader() = default;
    AssetSchemaLoader(const AssetSchemaLoader&) = delete;
    ~AssetSchemaLoader();

    AssetSchemaLoader& operator=(const AssetSchemaLoader&) = delete;

    bool load_registry(AssetRegistry reg, const View& toml, std::string& err);

private:
    bool load_asset_entries(std::string& err);

private:
    AssetRegistry mReg{};
    TOMLDocument mDoc{};
};

/// @brief Saves AssetRegistry to TOML
class AssetSchemaSaver
{
public:
    AssetSchemaSaver() = default;
    AssetSchemaSaver(const AssetSchemaSaver&) = delete;
    ~AssetSchemaSaver();

    AssetSchemaSaver& operator=(const AssetSchemaSaver&) = delete;

    bool save_registry(AssetRegistry registry, std::string& toml, std::string& err);

private:
    bool save_asset_entries(std::string& err);

private:
    AssetRegistry mReg{};
    TOMLWriter mWriter{};
};

AssetSchemaLoader::~AssetSchemaLoader()
{
    if (mDoc)
        TOMLDocument::destroy(mDoc);
}

bool AssetSchemaLoader::load_registry(AssetRegistry reg, const View& toml, std::string& err)
{
    mDoc = TOMLDocument::create();
    mReg = reg;

    if (!TOMLParser::parse(mDoc, toml, err))
        return false;

    TOMLValue registryTOML = mDoc.get(ASSET_SCHEMA_TABLE_LUDENS_ASSETS);
    if (!registryTOML || registryTOML.type() != TOML_TYPE_TABLE)
        return false;

    int32_t version;
    TOMLValue versionTOML = registryTOML[ASSET_SCHEMA_KEY_VERSION_MAJOR];
    if (!versionTOML || !versionTOML.get_i32(version) || version != LD_VERSION_MAJOR)
        return false;

    versionTOML = registryTOML[ASSET_SCHEMA_KEY_VERSION_MINOR];
    if (!versionTOML || !versionTOML.get_i32(version) || version != LD_VERSION_MINOR)
        return false;

    versionTOML = registryTOML[ASSET_SCHEMA_KEY_VERSION_PATCH];
    if (!versionTOML || !versionTOML.get_i32(version) || version != LD_VERSION_PATCH)
        return false;

    if (!load_asset_entries(err))
        return false;

    TOMLDocument::destroy(mDoc);
    mDoc = {};

    return true;
}

bool AssetSchemaLoader::load_asset_entries(std::string& err)
{
    for (int i = 0; i < (int)ASSET_TYPE_ENUM_COUNT; i++)
    {
        AssetType assetType = (AssetType)i;
        const char* typeCstr = get_asset_type_cstr(assetType);
        TOMLValue entryArrayTOML = mDoc.get(typeCstr);
        if (!entryArrayTOML || !entryArrayTOML.is_array())
            continue;

        int arraySize = entryArrayTOML.size();
        for (int j = 0; j < arraySize; j++)
        {
            TOMLValue entryTOML = entryArrayTOML[j];
            if (!entryTOML.is_table())
                continue;

            AssetEntry entry = {.type = assetType};
            TOMLValue idTOML = entryTOML[ASSET_SCHEMA_KEY_ENTRY_ID];
            TOMLValue uriTOML = entryTOML[ASSET_SCHEMA_KEY_ENTRY_URI];
            TOMLValue nameTOML = entryTOML[ASSET_SCHEMA_KEY_ENTRY_NAME];
            if (!uriTOML || !uriTOML.get_string(entry.uri) ||
                !nameTOML || !nameTOML.get_string(entry.name) ||
                !idTOML || !idTOML.get_u32(entry.id))
                continue;

            if (!mReg.register_asset_with_id(entry))
            {
                err = std::format("Asset ID {} is already in use, invalid schema", entry.id);
                return false;
            }
        }
    }

    return true;
}

AssetSchemaSaver::~AssetSchemaSaver()
{
    if (mWriter)
        TOMLWriter::destroy(mWriter);
}

bool AssetSchemaSaver::save_registry(AssetRegistry registry, std::string& toml, std::string& err)
{
    mReg = registry;

    mWriter = TOMLWriter::create();
    mWriter.begin();

    mWriter.begin_table(ASSET_SCHEMA_TABLE_LUDENS_ASSETS);
    mWriter.key(ASSET_SCHEMA_KEY_VERSION_MAJOR).value_u32(LD_VERSION_MAJOR);
    mWriter.key(ASSET_SCHEMA_KEY_VERSION_MINOR).value_u32(LD_VERSION_MINOR);
    mWriter.key(ASSET_SCHEMA_KEY_VERSION_PATCH).value_u32(LD_VERSION_PATCH);
    mWriter.end_table();

    if (!save_asset_entries(err))
        return false;

    mWriter.end(toml);
    TOMLWriter::destroy(mWriter);

    return true;
}

bool AssetSchemaSaver::save_asset_entries(std::string& err)
{
    for (int i = 0; i < (int)ASSET_TYPE_ENUM_COUNT; i++)
    {
        AssetType assetType = (AssetType)i;
        const char* typeCstr = get_asset_type_cstr(assetType);

        Vector<const AssetEntry*> entries;
        mReg.find_assets_by_type(assetType, entries);
        if (entries.empty())
            continue;

        mWriter.begin_array_table(typeCstr);

        for (const AssetEntry* entry : entries)
        {
            mWriter.begin_table();
            mWriter.key(ASSET_SCHEMA_KEY_ENTRY_ID).value_u32(entry->id);
            mWriter.key(ASSET_SCHEMA_KEY_ENTRY_URI).value_string(entry->uri);
            mWriter.key(ASSET_SCHEMA_KEY_ENTRY_NAME).value_string(entry->name);
            mWriter.end_table();
        }

        mWriter.end_array_table();
    }

    return true;
}

//
// Public API
//

bool AssetSchema::load_registry_from_file(AssetRegistry registry, const FS::Path& tomlPath, std::string& err)
{
    LD_PROFILE_SCOPE;

    Vector<byte> toml;
    if (!FS::read_file_to_vector(tomlPath, toml, err))
        return false;

    View tomlView((const char*)toml.data(), toml.size());
    AssetSchemaLoader loader;
    if (!loader.load_registry(registry, tomlView, err))
        return false;

    return true;
}

bool AssetSchema::save_registry(AssetRegistry registry, const FS::Path& savePath, std::string& err)
{
    LD_PROFILE_SCOPE;

    std::string toml;
    AssetSchemaSaver saver;
    if (!saver.save_registry(registry, toml, err))
        return false;

    return FS::write_file_and_swap_backup(savePath, View(toml.data(), toml.size()), err);
}

} // namespace LD