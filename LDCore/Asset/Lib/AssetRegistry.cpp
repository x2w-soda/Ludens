#include <Ludens/Asset/AssetRegistry.h>
#include <Ludens/DSA/HashMap.h>
#include <Ludens/DSA/URI.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Memory/Allocator.h>
#include <Ludens/Serial/SUIDTable.h>
#include <Ludens/System/FileSystem.h>

#include <format>

namespace LD {

static_assert(std::is_same_v<AssetID, SUID>);

struct AssetRegistryObj;

struct AssetEntryObj
{
    AssetRegistryObj* registry = nullptr;        // backwards link
    HashMap<std::string, std::string> filePaths; // physical relative paths to project root
    AssetType type;                              // asset type determines API
    SUID id;                                     // stable serial ID to identify the asset in project
};

/// @brief Asset registry implementation.
class AssetRegistryObj
{
public:
    AssetRegistryObj();
    AssetRegistryObj(const AssetRegistryObj&) = delete;
    ~AssetRegistryObj();

    AssetRegistryObj& operator=(const AssetRegistryObj&) = delete;

    AssetEntryObj* allocate_entry(AssetType type);
    void free_entry(AssetEntryObj* entry);
    AssetEntryObj* get_entry(SUID id);
    AssetEntryObj* get_entry_by_path(const std::string& path);
    AssetEntryObj* get_entry_by_name(const std::string& name);
    void get_entries_by_type(Vector<AssetEntry>& outEntries, AssetType type);
    void get_all_entries(Vector<AssetEntry>& outEntries);
    AssetEntry register_asset(SUIDRegistry idReg, SUID id, AssetType type, const std::string& path);
    void unregister_asset(SUIDRegistry idReg, SUID id);
    bool set_path(SUID id, const std::string& newPath);

    inline bool is_path_valid(const std::string& path, std::string& collidingPath) { return mTable.is_path_valid(path, collidingPath); }
    inline bool is_path_rename_valid(SUID id, const std::string& newPath) { return mTable.is_path_rename_valid(id, newPath); }

    std::string get_asset_name_by_id(SUID id);
    std::string get_asset_path_by_id(SUID id);

public:
    bool isDirty = false;

private:
    PoolAllocator mEntryPA;
    HashMap<SUID, AssetEntryObj*> mEntries;
    SUIDTable mTable;
};

AssetRegistryObj::AssetRegistryObj()
{
    PoolAllocatorInfo paI{};
    paI.isMultiPage = true;
    paI.blockSize = sizeof(AssetEntryObj);
    paI.pageSize = 16;
    paI.usage = MEMORY_USAGE_ASSET;
    mEntryPA = PoolAllocator::create(paI);
}

AssetRegistryObj::~AssetRegistryObj()
{
    for (auto it = mEntryPA.begin(); it; ++it)
    {
        static_cast<AssetEntryObj*>(it.data())->~AssetEntryObj();
    }

    PoolAllocator::destroy(mEntryPA);
}

AssetEntryObj* AssetRegistryObj::allocate_entry(AssetType type)
{
    AssetEntryObj* entry = (AssetEntryObj*)mEntryPA.allocate();
    new (entry) AssetEntryObj();

    entry->registry = this;
    entry->type = type;

    return entry;
}

void AssetRegistryObj::free_entry(AssetEntryObj* entry)
{
    entry->id = 0;
    entry->registry = nullptr;

    entry->~AssetEntryObj();
    mEntryPA.free(entry);
}

AssetEntryObj* AssetRegistryObj::get_entry(SUID id)
{
    auto it = mEntries.find(id);

    if (it == mEntries.end())
        return nullptr;

    return it->second;
}

AssetEntryObj* AssetRegistryObj::get_entry_by_path(const std::string& uriPath)
{
    SUID assetID = mTable.find_by_path(uriPath);
    if (!assetID)
        return nullptr;

    LD_ASSERT(mEntries.contains(assetID));
    return mEntries[assetID];
}

AssetEntryObj* AssetRegistryObj::get_entry_by_name(const std::string& name)
{
    SUID assetID = mTable.find_by_name(name);
    if (!assetID)
        return nullptr;

    LD_ASSERT(mEntries.contains(assetID));
    return mEntries[assetID];
}

void AssetRegistryObj::get_entries_by_type(Vector<AssetEntry>& outEntries, AssetType type)
{
    outEntries.clear();

    for (auto it = mEntryPA.begin(); it; ++it)
    {
        auto* entry = (AssetEntryObj*)it.data();
        LD_ASSERT(entry);

        if (entry->type == type)
            outEntries.push_back(AssetEntry(entry));
    }
}

void AssetRegistryObj::get_all_entries(Vector<AssetEntry>& outEntries)
{
    outEntries.clear();

    for (auto it : mEntries)
    {
        outEntries.push_back(AssetEntry(it.second));
    }
}

AssetEntry AssetRegistryObj::register_asset(SUIDRegistry idReg, SUID id, AssetType type, const std::string& path)
{
    URI uri(LD_ASSET_URI_SCHEME_AUTHORITY + path);
    std::string name = uri.stem_string();

    if (mTable.find_by_path(path) || mTable.find_by_name(name))
        return {};

    if (id)
    {
        // try registering with known ID.
        if (mEntries.contains(id) || (id.type() != SERIAL_TYPE_ASSET))
            return {};

        (void)idReg.try_get_suid(id);
    }
    else
    {
        id = idReg.get_suid(SERIAL_TYPE_ASSET);
    }

    AssetEntryObj* entry = allocate_entry(type);
    entry->id = id;

    mTable.register_id(id, path);
    mEntries[id] = entry;

    return AssetEntry(entry);
}

void AssetRegistryObj::unregister_asset(SUIDRegistry idReg, SUID id)
{
    if (!id || !mEntries.contains(id))
        return;

    AssetEntryObj* entry = mEntries[id];

    idReg.free_suid(id);
    mEntries.erase(id);
    mTable.unregister_id(id);
    free_entry(entry);
}

bool AssetRegistryObj::set_path(SUID id, const std::string& newPath)
{
    if (mTable.set_path(id, newPath))
    {
        isDirty = true;
        return true;
    }

    return false;
}

std::string AssetRegistryObj::get_asset_name_by_id(SUID id)
{
    std::string name;
    (void)mTable.get_name(id, name);
    return name;
}

std::string AssetRegistryObj::get_asset_path_by_id(SUID id)
{
    std::string path;
    (void)mTable.get_path(id, path);
    return path;
}

//
// Public API
//

SUID AssetEntry::get_id()
{
    return mObj->id;
}

AssetType AssetEntry::get_type()
{
    return mObj->type;
}

std::string AssetEntry::get_name()
{
    return mObj->registry->get_asset_name_by_id(mObj->id);
}

std::string AssetEntry::get_path()
{
    return mObj->registry->get_asset_path_by_id(mObj->id);
}

bool AssetEntry::set_path(const std::string& path)
{
    return mObj->registry->set_path(mObj->id, path);
}

Vector<std::string> AssetEntry::get_file_path_keys()
{
    Vector<std::string> keys;
    for (auto it : mObj->filePaths)
        keys.push_back(it.first);

    return keys;
}

std::string AssetEntry::get_file_path(const std::string& key)
{
    auto it = mObj->filePaths.find(key);

    if (it == mObj->filePaths.end())
        return {};

    return it->second;
}

void AssetEntry::set_file_path(const std::string& key, const std::string& filePath)
{
    // override or append
    mObj->filePaths[key] = filePath;

    mObj->registry->isDirty = true;
}

AssetRegistry AssetRegistry::create()
{
    auto* obj = heap_new<AssetRegistryObj>(MEMORY_USAGE_ASSET);

    return AssetRegistry(obj);
}

void AssetRegistry::destroy(AssetRegistry registry)
{
    auto* obj = registry.unwrap();

    heap_delete<AssetRegistryObj>(obj);
}

AssetEntry AssetRegistry::register_asset_with_id(SUIDRegistry idReg, SUID id, AssetType type, const std::string& uriPath)
{
    return mObj->register_asset(idReg, id, type, uriPath);
}

AssetEntry AssetRegistry::register_asset(SUIDRegistry idReg, AssetType type, const std::string& uriPath)
{
    return mObj->register_asset(idReg, (SUID)0, type, uriPath);
}

void AssetRegistry::unregister_asset(SUIDRegistry idReg, SUID id)
{
    mObj->unregister_asset(idReg, id);
}

bool AssetRegistry::is_path_valid(const std::string& path, std::string& collidingPath)
{
    collidingPath.clear();

    return mObj->is_path_valid(path, collidingPath);
}

bool AssetRegistry::is_uri_valid(const URI& uri, std::string& collidingPath)
{
    if (uri.scheme() != LD_ASSET_URI_SCHEME || uri.authority() != LD_ASSET_URI_AUTHORITY)
        return false;

    return is_path_valid(uri.path_string(), collidingPath);
}

bool AssetRegistry::is_dirty()
{
    return mObj->isDirty;
}

void AssetRegistry::clear_dirty()
{
    mObj->isDirty = false;
}

AssetEntry AssetRegistry::get_entry(SUID id)
{
    return AssetEntry(mObj->get_entry(id));
}

AssetEntry AssetRegistry::get_entry_by_path(const std::string& uriPath)
{
    return AssetEntry(mObj->get_entry_by_path(uriPath));
}

AssetEntry AssetRegistry::get_entry_by_name(const std::string& name)
{
    return AssetEntry(mObj->get_entry_by_name(name));
}

void AssetRegistry::get_entries_by_type(Vector<AssetEntry>& outEntries, AssetType type)
{
    return mObj->get_entries_by_type(outEntries, type);
}

void AssetRegistry::get_all_entries(Vector<AssetEntry>& outEntries)
{
    return mObj->get_all_entries(outEntries);
}

} // namespace LD
