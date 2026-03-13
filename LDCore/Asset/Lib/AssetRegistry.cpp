#include <Ludens/Asset/AssetRegistry.h>
#include <Ludens/DSA/HashMap.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Memory/Allocator.h>

namespace LD {

static_assert(std::is_same_v<AssetID, SUID>);

struct AssetEntryObj
{
    std::string name;                           // user-facing asset name
    std::string uri;                            // relative path to project root
    HashMap<std::string, std::string> extraURI; // relative paths to project root; non-empty if asset maps to more than one file.
    AssetType type;                             // asset type determines API
    SUID id;                                    // stable serial ID, identifies the asset throughout the project
};

/// @brief Asset registry implementation.
class AssetRegistryObj
{
public:
    AssetRegistryObj();
    AssetRegistryObj(const AssetRegistryObj&) = delete;
    ~AssetRegistryObj();

    AssetRegistryObj& operator=(const AssetRegistryObj&) = delete;

    AssetEntryObj* allocate_entry(AssetType type, SUID id);
    AssetEntryObj* get_entry(SUID id);
    void get_entries_by_type(Vector<AssetEntry>& outEntries, AssetType type);
    void get_all_entries(Vector<AssetEntry>& outEntries);
    AssetEntry register_asset(SUID id, AssetType type, const std::string& uri, const std::string& name);
    void unregister_asset(SUID id);

private:
    PoolAllocator mEntryPA;
    HashMap<SUID, AssetEntryObj*> mEntries;
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

AssetEntryObj* AssetRegistryObj::allocate_entry(AssetType type, SUID id)
{
    AssetEntryObj* entry = (AssetEntryObj*)mEntryPA.allocate();
    new (entry) AssetEntryObj();

    entry->type = type;
    entry->id = id;

    return entry;
}

AssetEntryObj* AssetRegistryObj::get_entry(SUID id)
{
    auto it = mEntries.find(id);

    if (it == mEntries.end())
        return nullptr;

    return it->second;
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

AssetEntry AssetRegistryObj::register_asset(SUID id, AssetType type, const std::string& uri, const std::string& name)
{
    if (id)
    {
        // try registering with known ID.
        if (mEntries.contains(id) || (id.type() != SERIAL_TYPE_ASSET) || !SUIDRegistry::try_get_suid(id))
            return {};
    }
    else
    {
        id = SUIDRegistry::get_suid(SERIAL_TYPE_ASSET);
    }

    AssetEntryObj* entry = allocate_entry(type, id);
    entry->uri = uri;
    entry->name = name;

    mEntries[id] = entry;

    return AssetEntry(entry);
}

void AssetRegistryObj::unregister_asset(SUID id)
{
    if (!id || !mEntries.contains(id))
        return;

    AssetEntryObj* entry = mEntries[id];
    entry->id = 0;
    mEntryPA.free(entry);

    mEntries.erase(id);
    SUIDRegistry::free_suid(id);
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
    return mObj->name;
}

std::string AssetEntry::get_uri()
{
    return mObj->uri;
}

void AssetEntry::set_uri(const std::string& uri)
{
    mObj->uri = uri;
}

Vector<std::string> AssetEntry::get_extra_uri_keys()
{
    Vector<std::string> keys;
    for (auto it : mObj->extraURI)
        keys.push_back(it.first);

    return keys;
}

std::string AssetEntry::get_extra_uri(const std::string& key)
{
    auto it = mObj->extraURI.find(key);

    if (it == mObj->extraURI.end())
        return {};

    return it->second;
}

void AssetEntry::set_extra_uri(const std::string& key, const std::string& uri)
{
    // override or append
    mObj->extraURI[key] = uri;
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

AssetEntry AssetRegistry::register_asset_with_id(SUID id, AssetType type, const std::string& uri, const std::string& name)
{
    return mObj->register_asset(id, type, uri, name);
}

AssetEntry AssetRegistry::register_asset(AssetType type, const std::string& uri, const std::string& name)
{
    return mObj->register_asset((SUID)0, type, uri, name);
}

void AssetRegistry::unregister_asset(SUID id)
{
    mObj->unregister_asset(id);
}

AssetEntry AssetRegistry::get_entry(SUID id)
{
    return AssetEntry(mObj->get_entry(id));
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