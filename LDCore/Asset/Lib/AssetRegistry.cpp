#include <Ludens/Asset/AssetRegistry.h>
#include <Ludens/DSA/HashMap.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Memory/Allocator.h>

namespace LD {

static_assert(std::is_same_v<AssetID, uint32_t>);

/// @brief Asset registry implementation.
class AssetRegistryObj
{
public:
    AssetRegistryObj() = default;
    AssetRegistryObj(const AssetRegistryObj&) = delete;
    ~AssetRegistryObj();

    AssetRegistryObj& operator=(const AssetRegistryObj&) = delete;

    AssetEntry* allocate_entry(AssetType type, SUID id);
    AssetEntry* get_entry(SUID id);
    PoolAllocator get_pa(AssetType type);
    PoolAllocator get_or_create_pa(AssetType type);
    bool register_asset_with_id(const AssetEntry& entry);
    SUID register_asset(AssetType type, const std::string& uri, const std::string& name);
    void unregister_asset(SUID id);

private:
    HashMap<AssetType, PoolAllocator> mEntryPAs;
    HashMap<SUID, AssetEntry*> mEntries;
};

AssetRegistryObj::~AssetRegistryObj()
{
    for (auto ite : mEntryPAs)
    {
        PoolAllocator pa = ite.second;

        for (auto entryIte = pa.begin(); entryIte; ++entryIte)
        {
            static_cast<AssetEntry*>(entryIte.data())->~AssetEntry();
        }

        PoolAllocator::destroy(pa);
    }
}

AssetEntry* AssetRegistryObj::allocate_entry(AssetType type, SUID id)
{
    PoolAllocator pa = get_or_create_pa(type);
    AssetEntry* entry = (AssetEntry*)pa.allocate();
    new (entry) AssetEntry();

    entry->type = type;
    entry->id = id;

    return entry;
}

AssetEntry* AssetRegistryObj::get_entry(SUID id)
{
    auto ite = mEntries.find(id);

    if (ite == mEntries.end())
        return nullptr;

    return ite->second;
}

PoolAllocator AssetRegistryObj::get_pa(AssetType type)
{
    if (mEntryPAs.contains(type))
        return mEntryPAs[type];

    return {};
}

PoolAllocator AssetRegistryObj::get_or_create_pa(AssetType type)
{
    if (mEntryPAs.contains(type))
        return mEntryPAs[type];

    PoolAllocatorInfo paI{};
    paI.isMultiPage = true;
    paI.blockSize = sizeof(AssetEntry);
    paI.pageSize = 16;
    paI.usage = MEMORY_USAGE_ASSET;
    return mEntryPAs[type] = PoolAllocator::create(paI);
}

bool AssetRegistryObj::register_asset_with_id(const AssetEntry& entry)
{
    if (mEntries.contains(entry.id) || !try_get_suid(entry.id))
        return false;

    AssetEntry* pEntry = allocate_entry(entry.type, entry.id);
    pEntry->uri = entry.uri;
    pEntry->name = entry.name;

    mEntries[entry.id] = pEntry;

    return true;
}

SUID AssetRegistryObj::register_asset(AssetType type, const std::string& uri, const std::string& name)
{
    SUID id = get_suid();
    AssetEntry* pEntry = allocate_entry(type, id);
    pEntry->uri = uri;
    pEntry->name = name;

    mEntries[id] = pEntry;

    return id;
}

void AssetRegistryObj::unregister_asset(SUID id)
{
    if (id == 0 || !mEntries.contains(id))
        return;

    AssetEntry* entry = mEntries[id];
    PoolAllocator pa = get_or_create_pa(entry->type);
    entry->id = 0;
    pa.free(entry);

    mEntries.erase(id);
    free_suid(id);
}

//
// Public API
//

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

bool AssetRegistry::register_asset_with_id(const AssetEntry& entry)
{
    return mObj->register_asset_with_id(entry);
}

SUID AssetRegistry::register_asset(AssetType type, const std::string& uri, const std::string& name)
{
    return mObj->register_asset(type, uri, name);
}

void AssetRegistry::unregister_asset(SUID id)
{
    mObj->unregister_asset(id);
}

const AssetEntry* AssetRegistry::find_asset(SUID id)
{
    return mObj->get_entry(id);
}

void AssetRegistry::find_assets_by_type(AssetType type, Vector<const AssetEntry*>& entries)
{
    entries.clear();

    PoolAllocator pa = mObj->get_pa(type);
    if (!pa)
        return;

    for (auto ite = pa.begin(); ite; ++ite)
    {
        auto* entry = (const AssetEntry*)ite.data();
        LD_ASSERT(entry && entry->type == type);
        entries.push_back(entry);
    }
}

} // namespace LD