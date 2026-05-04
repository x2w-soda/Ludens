#include <Ludens/Asset/AssetSchema.h>
#include <Ludens/DSA/ViewUtil.h>
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

    bool load_registry(AssetRegistry reg, SUIDRegistry idReg, const View& toml, std::string& err);

private:
    bool load_asset_entries(std::string& err);

private:
    AssetRegistry mReg{};
    TOMLReader mReader{};
    SUIDRegistry mIDReg{};
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
    if (mReader)
        TOMLReader::destroy(mReader);
}

bool AssetSchemaLoader::load_registry(AssetRegistry reg, SUIDRegistry idReg, const View& toml, std::string& err)
{
    mReg = reg;
    mReader = TOMLReader::create(toml, err);
    mIDReg = idReg;

    if (!mReader || !mReader.enter_table(ASSET_SCHEMA_TABLE_LUDENS_ASSETS))
        return false;

    int32_t version;
    if (!mReader.read_i32(ASSET_SCHEMA_KEY_VERSION_MAJOR, version) || version != LD_VERSION_MAJOR)
        return false;

    if (!mReader.read_i32(ASSET_SCHEMA_KEY_VERSION_MINOR, version) || version != LD_VERSION_MINOR)
        return false;

    if (!mReader.read_i32(ASSET_SCHEMA_KEY_VERSION_PATCH, version) || version != LD_VERSION_PATCH)
        return false;

    mReader.exit();

    if (!load_asset_entries(err))
        return false;

    TOMLReader::destroy(mReader);
    mReader = {};

    return true;
}

bool AssetSchemaLoader::load_asset_entries(std::string& err)
{
    int count;
    if (!mReader.enter_array(ASSET_SCHEMA_KEY_ENTRY, count))
        return true; // zero asset entries

    bool isValid = true;
    AssetType assetType;
    AssetEntry entry{};
    Vector<std::string> keys;
    std::string entryPath;
    std::string entryType;
    SUID entryID;

    for (int i = 0; isValid && i < count; i++)
    {
        if (!mReader.enter_table(i))
            continue;

        if (!mReader.read_string(ASSET_SCHEMA_KEY_ENTRY_PATH, entryPath) ||
            !mReader.read_string(ASSET_SCHEMA_KEY_ENTRY_TYPE, entryType) ||
            !mReader.read_suid(ASSET_SCHEMA_KEY_ENTRY_ID, entryID) ||
            !get_cstr_asset_type(entryType.c_str(), assetType))
        {
            mReader.exit();
            continue;
        }

        if (isValid && entryID.type() != SERIAL_TYPE_ASSET)
        {
            err = std::format("Asset [{}] has invalid ID {}", entryPath, entryID);
            isValid = false;
        }

        if (isValid && !(entry = mReg.register_asset_with_id(mIDReg, entryID, assetType, entryPath)))
        {
            err = std::format("Asset ID 0x{} is already in use, invalid schema", entryID.to_string());
            isValid = false;
        }

        if (isValid && mReader.enter_table(ASSET_SCHEMA_KEY_ENTRY_FILE_PATHS))
        {
            mReader.get_keys(keys);

            for (const std::string& key : keys)
            {
                if (mReader.read_string(key.c_str(), entryPath))
                    entry.set_file_path(key, entryPath);
            }

            mReader.exit();
        }

        mReader.exit();
    }

    mReader.exit();

    return isValid;
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
    mWriter.key(ASSET_SCHEMA_KEY_VERSION_MAJOR).write_u32(LD_VERSION_MAJOR);
    mWriter.key(ASSET_SCHEMA_KEY_VERSION_MINOR).write_u32(LD_VERSION_MINOR);
    mWriter.key(ASSET_SCHEMA_KEY_VERSION_PATCH).write_u32(LD_VERSION_PATCH);
    mWriter.end_table();

    if (!save_asset_entries(err))
        return false;

    mWriter.end(toml);
    TOMLWriter::destroy(mWriter);
    mWriter = {};

    return true;
}

bool AssetSchemaSaver::save_asset_entries(std::string& err)
{
    Vector<std::string> keys;
    Vector<AssetEntry> entries;
    mReg.get_all_entries(entries);
    if (entries.empty())
        return true;

    mWriter.begin_array_table(ASSET_SCHEMA_KEY_ENTRY);

    for (AssetEntry entry : entries)
    {
        const char* typeCstr = get_asset_type_cstr(entry.get_type());

        mWriter.begin_table();
        mWriter.key(ASSET_SCHEMA_KEY_ENTRY_ID).write_u32(entry.get_id());
        mWriter.key(ASSET_SCHEMA_KEY_ENTRY_PATH).write_string(entry.get_path());
        mWriter.key(ASSET_SCHEMA_KEY_ENTRY_TYPE).write_string(typeCstr);

        keys = entry.get_file_path_keys();
        if (!keys.empty())
        {
            mWriter.begin_table(ASSET_SCHEMA_KEY_ENTRY_FILE_PATHS);
            for (const std::string& key : keys)
                mWriter.key(key).write_string(entry.get_file_path(key));
            mWriter.end_table();
        }

        mWriter.end_table();
    }

    mWriter.end_array_table();

    return true;
}

//
// Public API
//

bool AssetSchema::load_registry_from_file(AssetRegistry registry, SUIDRegistry idRegistry, const FS::Path& tomlPath, std::string& err)
{
    LD_PROFILE_SCOPE;

    Vector<byte> toml;
    if (!FS::read_file_to_vector(tomlPath, toml, err))
        return false;

    AssetSchemaLoader loader;
    if (!loader.load_registry(registry, idRegistry, view(toml), err))
        return false;

    return true;
}

bool AssetSchema::save_registry_to_string(AssetRegistry registry, std::string& saveTOML, std::string& err)
{
    LD_PROFILE_SCOPE;

    AssetSchemaSaver saver;
    if (!saver.save_registry(registry, saveTOML, err))
        return false;

    return true;
}

bool AssetSchema::save_registry(AssetRegistry registry, const FS::Path& savePath, std::string& err)
{
    LD_PROFILE_SCOPE;

    std::string toml;
    if (!save_registry_to_string(registry, toml, err))
        return false;

    return FS::write_file_and_swap_backup(savePath, view(toml), err);
}

std::string AssetSchema::create_empty()
{
    AssetRegistry reg = AssetRegistry::create();

    std::string toml, err;
    bool success = AssetSchema::save_registry_to_string(reg, toml, err);
    LD_ASSERT(success);

    return toml;
}

} // namespace LD