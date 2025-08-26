#include <Ludens/DataRegistry/DataComponent.h>
#include <Ludens/DataRegistry/DataRegistry.h>
#include <Ludens/Header/Types.h>
#include <Ludens/System/Memory.h>
#include <unordered_map>
#include <unordered_set>
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
    { COMPONENT_TYPE_DATA,       sizeof(ComponentBase),      "DataComponent",      nullptr,            nullptr },
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
    ComponentBase* base;
    ComponentScriptSlot* script;
    void* comp;
};

struct DataRegistryObj
{
    std::unordered_map<ComponentType, PoolAllocator> componentPAs;
    std::unordered_map<DUID, DataComponentEntry> components;
    std::unordered_set<DUID> roots;
    PoolAllocator componentBasePA;
    PoolAllocator scriptPA;
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

    void add_child(ComponentBase* parent, ComponentBase* child);
};

void DataRegistryObj::add_child(ComponentBase* parent, ComponentBase* child)
{
    child->parent = parent;

    if (parent)
    {
        child->next = parent->child;
        parent->child = child;
    }
}

DataRegistry DataRegistry::create()
{
    DataRegistryObj* obj = heap_new<DataRegistryObj>(MEMORY_USAGE_MISC);

    PoolAllocatorInfo paI{};
    paI.blockSize = sizeof(ComponentBase);
    paI.pageSize = 1024;
    paI.isMultiPage = true;
    paI.usage = MEMORY_USAGE_MISC;
    obj->componentBasePA = PoolAllocator::create(paI);

    paI.blockSize = sizeof(ComponentScriptSlot);
    paI.pageSize = 128;
    paI.isMultiPage = true;
    paI.usage = MEMORY_USAGE_MISC;
    obj->scriptPA = PoolAllocator::create(paI);

    return {obj};
}

void DataRegistry::destroy(DataRegistry registry)
{
    DataRegistryObj* obj = registry;

    for (auto ite = obj->componentBasePA.begin(); ite; ++ite)
    {
        ComponentBase* base = (ComponentBase*)ite.data();

        if (base->name)
            heap_free(base->name);
    }

    PoolAllocator::destroy(obj->componentBasePA);
    PoolAllocator::destroy(obj->scriptPA);

    for (auto ite : obj->componentPAs)
    {
        PoolAllocator::destroy(ite.second);
    }

    heap_delete<DataRegistryObj>(obj);
}

DUID DataRegistry::create_component(ComponentType type, const char* name, DUID parentID)
{
    if (!mObj->componentPAs.contains(type))
    {
        PoolAllocatorInfo paI{};
        paI.blockSize = get_component_byte_size(type);
        paI.pageSize = 1024;
        paI.isMultiPage = true;
        paI.usage = MEMORY_USAGE_MISC;
        mObj->componentPAs[type] = PoolAllocator::create(paI);
    }

    // allocate base members
    ComponentBase* base = (ComponentBase*)mObj->componentBasePA.allocate();
    base->name = heap_strdup(name, MEMORY_USAGE_MISC);
    base->next = nullptr;
    base->child = nullptr;
    base->parent = nullptr;
    base->type = type;
    base->id = mObj->get_id();

    if (parentID)
    {
        ComponentBase* parent = mObj->components[parentID].base;
        mObj->add_child(parent, base);
    }
    else
    {
        mObj->roots.insert(base->id);
    }

    // allocate component type
    mObj->components[base->id].base = base;
    mObj->components[base->id].comp = mObj->componentPAs[type].allocate();
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
    LD_ASSERT(mObj->componentPAs.contains(entry.base->type));
    LD_ASSERT(!entry.script);

    mObj->componentPAs[entry.base->type].free(entry.comp);
    mObj->componentBasePA.free(entry.base);
    mObj->components.erase(ite);
}

ComponentScriptSlot* DataRegistry::create_component_script_slot(DUID compID, AUID assetID)
{
    auto ite = mObj->components.find(compID);
    if (ite == mObj->components.end())
        return nullptr;

    LD_ASSERT(!ite->second.script); // script slot already exists

    auto* script = (ComponentScriptSlot*)mObj->scriptPA.allocate();
    script->assetID = assetID;
    script->componentID = compID;
    script->isEnabled = true;

    ite->second.script = script;

    return script;
}

void DataRegistry::destroy_component_script_slot(DUID compID)
{
    auto ite = mObj->components.find(compID);
    if (ite == mObj->components.end())
        return;

    ComponentScriptSlot* script = ite->second.script;
    LD_ASSERT(script); // script slot does not exist

    mObj->scriptPA.free(script);

    ite->second.script = nullptr;
}

ComponentBase* DataRegistry::get_component_base(DUID id)
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

void DataRegistry::get_root_components(std::vector<DUID>& roots)
{
    roots.resize(mObj->roots.size());

    int i = 0;

    for (DUID compID : mObj->roots)
        roots[i++] = compID;
}

PoolAllocator::Iterator DataRegistry::get_components(ComponentType type)
{
    LD_ASSERT(mObj->componentPAs.contains(type));

    return mObj->componentPAs[type].begin();
}

ComponentScriptSlot* DataRegistry::get_component_script(DUID comp)
{
    auto ite = mObj->components.find(comp);

    if (ite == mObj->components.end())
        return nullptr;

    return ite->second.script; // nullable
}

PoolAllocator::Iterator DataRegistry::get_component_scripts()
{
    return mObj->scriptPA.begin();
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