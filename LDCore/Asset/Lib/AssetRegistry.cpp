#include <Ludens/Asset/AssetRegistry.h>
#include <Ludens/DSA/HashMap.h>
#include <Ludens/DSA/HashSet.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Memory/Allocator.h>

namespace LD {

static_assert(std::is_same_v<AUID, uint32_t>);

/// @brief Asset registry implementation.
class AssetRegistryObj
{
public:
    AssetRegistryObj() = default;
    AssetRegistryObj(const AssetRegistryObj&) = delete;
    ~AssetRegistryObj();

    AssetRegistryObj& operator=(const AssetRegistryObj&) = delete;

    AUID get_auid();
    AssetEntry* allocate_entry(AssetType type, AUID auid);
    AssetEntry* get_entry(AUID auid);
    PoolAllocator get_pa(AssetType type);
    PoolAllocator get_or_create_pa(AssetType type);
    bool register_asset_with_id(const AssetEntry& entry);
    AUID register_asset(AssetType type, const std::string& uri, const std::string& name);
    void unregister_asset(AUID auid);

    inline void set_auid_counter(uint32_t counter) { mAUIDCounter = counter; }
    inline uint32_t get_auid_counter() { return mAUIDCounter; }

private:
    std::unordered_map<AssetType, PoolAllocator> mEntryPAs;
    std::unordered_map<AUID, AssetEntry*> mEntries;
    std::unordered_set<AUID> mAUIDInUse;
    uint32_t mAUIDCounter = 1;
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

// This is assumed to always return a valid ID,
// it is unlikely that the 32-bit ID space is exhausted.
AUID AssetRegistryObj::get_auid()
{
    if (mAUIDCounter == 0)
        mAUIDCounter++;

    while (mAUIDInUse.contains(mAUIDCounter))
        mAUIDCounter++;

    AUID auid = mAUIDCounter++;
    mAUIDInUse.insert(auid);

    return auid;
}

AssetEntry* AssetRegistryObj::allocate_entry(AssetType type, AUID auid)
{
    PoolAllocator pa = get_or_create_pa(type);
    AssetEntry* entry = (AssetEntry*)pa.allocate();
    new (entry) AssetEntry();

    entry->type = type;
    entry->id = auid;

    return entry;
}

AssetEntry* AssetRegistryObj::get_entry(AUID auid)
{
    auto ite = mEntries.find(auid);

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
    if (mAUIDInUse.contains(entry.id) || mEntries.contains(entry.id))
        return false;

    AssetEntry* pEntry = allocate_entry(entry.type, entry.id);
    pEntry->uri = entry.uri;
    pEntry->name = entry.name;

    mAUIDInUse.insert(entry.id);
    mEntries[entry.id] = pEntry;

    return true;
}

AUID AssetRegistryObj::register_asset(AssetType type, const std::string& uri, const std::string& name)
{
    AUID auid = get_auid();
    AssetEntry* pEntry = allocate_entry(type, auid);
    pEntry->uri = uri;
    pEntry->name = name;

    mEntries[auid] = pEntry;

    return auid;
}

void AssetRegistryObj::unregister_asset(AUID auid)
{
    if (auid == 0 || !mAUIDInUse.contains(auid) || !mEntries.contains(auid))
        return;

    AssetEntry* entry = mEntries[auid];
    PoolAllocator pa = get_or_create_pa(entry->type);
    pa.free(entry);

    mEntries.erase(auid);
    mAUIDInUse.erase(auid);
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

AUID AssetRegistry::register_asset(AssetType type, const std::string& uri, const std::string& name)
{
    return mObj->register_asset(type, uri, name);
}

void AssetRegistry::unregister_asset(AUID auid)
{
    mObj->unregister_asset(auid);
}

void AssetRegistry::set_auid_counter(uint32_t auidCounter)
{
    mObj->set_auid_counter(auidCounter);
}

uint32_t AssetRegistry::get_auid_counter()
{
    return mObj->get_auid_counter();
}

const AssetEntry* AssetRegistry::find_asset(AUID auid)
{
    return mObj->get_entry(auid);
}

void AssetRegistry::find_assets_by_type(AssetType type, std::vector<const AssetEntry*>& entries)
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