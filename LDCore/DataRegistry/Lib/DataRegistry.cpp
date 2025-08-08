#include <Ludens/DataRegistry/DataComponent.h>
#include <Ludens/DataRegistry/DataRegistry.h>
#include <Ludens/Header/Types.h>
#include <Ludens/System/Memory.h>
#include <unordered_map>
#include <vector>

namespace LD {

static Transform* get_transform(void* comp);
static Transform* get_mesh_transform(void* comp);
static RUID get_mesh_ruid(void* comp);

Transform* get_transform(void* comp)
{
    return &((TransformComponent*)comp)->transform;
}

static Transform* get_mesh_transform(void* comp)
{
    return &((MeshComponent*)comp)->transform;
}

static RUID get_mesh_ruid(void* comp)
{
    return ((MeshComponent*)comp)->ruid;
}

struct ComponentEntry
{
    ComponentType type;
    size_t byteSize;
    const char* typeName;
    Transform* (*get_transform)(void* comp);
    RUID (*get_ruid)(void* comp);
};

// clang-format off
static ComponentEntry sComponentTable[] = {
    { COMPONENT_TYPE_DATA,       sizeof(DataComponent),      "DataComponent",      nullptr,            nullptr },
    { COMPONENT_TYPE_TRANSFORM,  sizeof(TransformComponent), "TransformComponent", get_transform,      nullptr },
    { COMPONENT_TYPE_MESH,       sizeof(MeshComponent),      "MeshComponent",      get_mesh_transform, get_mesh_ruid },
    { COMPONENT_TYPE_TEXTURE_2D, sizeof(Texture2DComponent), "Texture2DComponent", nullptr,            nullptr },
};
// clang-format on

static_assert(sizeof(sComponentTable) / sizeof(*sComponentTable) == COMPONENT_TYPE_ENUM_COUNT);
static_assert(LD::IsDataComponent<TransformComponent>);
static_assert(LD::IsDataComponent<MeshComponent>);
static_assert(LD::IsDataComponent<Texture2DComponent>);

size_t get_component_byte_size(ComponentType type)
{
    return sComponentTable[(int)type].byteSize;
}

const char* get_component_type_name(ComponentType type)
{
    return sComponentTable[(int)type].typeName;
}

struct DataComponentEntry
{
    DataComponent* base;
    DataComponentScript* script;
    void* comp;
};

struct DataRegistryObj
{
    std::unordered_map<ComponentType, PoolAllocator> allocators;
    std::unordered_map<DUID, DataComponentEntry> components;
    uint32_t idCounter = 0;

    DUID get_id()
    {
        do
        {
            // linear probing for next valid DUID,
            // will only run into collisions after
            // the unsigned integer counter wraps.
            idCounter++;
        } while (idCounter == 0 || components.contains(idCounter));

        return idCounter;
    }
};

DataRegistry DataRegistry::create()
{
    DataRegistryObj* obj = heap_new<DataRegistryObj>(MEMORY_USAGE_MISC);

    PoolAllocatorInfo paI{};
    paI.blockSize = get_component_byte_size(COMPONENT_TYPE_DATA);
    paI.pageSize = 1024;
    paI.isMultiPage = true;
    paI.usage = MEMORY_USAGE_MISC;
    obj->allocators[COMPONENT_TYPE_DATA] = PoolAllocator::create(paI);

    return {obj};
}

void DataRegistry::destroy(DataRegistry registry)
{
    DataRegistryObj* obj = registry;

    for (auto ite : obj->allocators)
    {
        PoolAllocator::destroy(ite.second);
    }

    heap_delete<DataRegistryObj>(obj);
}

DUID DataRegistry::create_component(ComponentType type, const char* name)
{
    if (!mObj->allocators.contains(type))
    {
        PoolAllocatorInfo paI{};
        paI.blockSize = get_component_byte_size(type);
        paI.pageSize = 1024;
        paI.isMultiPage = true;
        paI.usage = MEMORY_USAGE_MISC;
        mObj->allocators[type] = PoolAllocator::create(paI);
    }

    // allocate base members
    DataComponent* base = (DataComponent*)mObj->allocators[COMPONENT_TYPE_DATA].allocate();
    base->name = heap_strdup(name, MEMORY_USAGE_MISC);
    base->next = nullptr;
    base->child = nullptr;
    base->type = type;
    base->id = mObj->get_id();

    // allocate component type
    mObj->components[base->id].base = base;
    mObj->components[base->id].comp = mObj->allocators[type].allocate();
    mObj->components[base->id].script = nullptr;

    return base->id;
}

void DataRegistry::destroy_component(DUID comp)
{
    auto ite = mObj->components.find(comp);

    if (ite == mObj->components.end())
        return;

    DataComponentEntry& entry = ite->second;
    LD_ASSERT(entry.base);
    LD_ASSERT(mObj->allocators.contains(entry.base->type));
    LD_ASSERT(!entry.script);

    mObj->allocators[entry.base->type].free(entry.comp);
    mObj->allocators[COMPONENT_TYPE_DATA].free(entry.base);
    mObj->components.erase(ite);
}

const DataComponent* DataRegistry::get_component_base(DUID id)
{
    auto ite = mObj->components.find(id);
    if (ite == mObj->components.end())
        return nullptr;

    return ite->second.base;
}

void* DataRegistry::get_component(DUID id, ComponentType& type)
{
    auto ite = mObj->components.find(id);
    if (ite == mObj->components.end())
        return nullptr;

    type = ite->second.base->type;
    return ite->second.comp;
}

PoolAllocator::Iterator DataRegistry::get_components(ComponentType type)
{
    LD_ASSERT(mObj->allocators.contains(type));

    return mObj->allocators[type].begin();
}

Transform* DataRegistry::get_component_transform(DUID comp)
{
    auto ite = mObj->components.find(comp);
    if (ite == mObj->components.end())
        return nullptr;

    ComponentType type = ite->second.base->type;

    if (!sComponentTable[type].get_transform)
        return nullptr;

    return sComponentTable[type].get_transform(ite->second.comp);
}

RUID DataRegistry::get_component_ruid(DUID comp)
{
    auto ite = mObj->components.find(comp);
    if (ite == mObj->components.end())
        return (RUID)0;

    ComponentType type = ite->second.base->type;

    if (!sComponentTable[type].get_ruid)
        return (RUID)0;

    return sComponentTable[type].get_ruid(ite->second.comp);
}

} // namespace LD