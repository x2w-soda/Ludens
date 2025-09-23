#include <Ludens/DataRegistry/DataComponent.h>
#include <Ludens/DataRegistry/DataRegistry.h>
#include <Ludens/Header/Types.h>
#include <Ludens/Log/Log.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/System/Memory.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace LD {

static Log sLog("DataRegistry");
static bool duplicate_subtree(DataRegistry dup, DataRegistry orig, CUID origRoot);
static Transform* get_transform(void* comp);
static Transform* get_mesh_transform(void* comp);
static AUID get_mesh_auid(void* comp);

struct ComponentMeta
{
    ComponentType type;
    size_t byteSize;
    const char* typeName;
    Transform* (*get_transform)(void* comp);
    AUID (*get_auid)(void* comp);
};

// clang-format off
static ComponentMeta sComponentTable[] = {
    { COMPONENT_TYPE_DATA,       sizeof(ComponentBase),      "DataComponent",      nullptr,             nullptr },
    { COMPONENT_TYPE_TRANSFORM,  sizeof(TransformComponent), "TransformComponent", &get_transform,      nullptr },
    { COMPONENT_TYPE_MESH,       sizeof(MeshComponent),      "MeshComponent",      &get_mesh_transform, &get_mesh_auid },
    { COMPONENT_TYPE_TEXTURE_2D, sizeof(Texture2DComponent), "Texture2DComponent", nullptr,             nullptr },
};
// clang-format on

static bool duplicate_subtree(DataRegistry dup, DataRegistry orig, CUID origRoot)
{
    const ComponentBase* origBase = orig.get_component_base(origRoot);
    const ComponentBase* origParentBase = origBase->parent;
    CUID parentID = origParentBase ? origParentBase->id : 0;
    CUID dupID = dup.create_component(origBase->type, origBase->name, parentID, origBase->id);

    if (dupID != origBase->id)
    {
        sLog.error("failed to duplicate {}", origBase->name);
        return false;
    }

    // duplicate fields
    ComponentBase* dupBase = dup.get_component_base(dupID);
    dupBase->flags = origBase->flags;
    dupBase->localMat4 = origBase->localMat4;
    dupBase->worldMat4 = origBase->worldMat4;

    // duplicate type-sepcific fields
    ComponentType type;
    void* dupComp = dup.get_component(dupID, type);
    void* origComp = orig.get_component(dupID, type);
    memcpy(dupComp, origComp, sComponentTable[(int)type].byteSize);

    // duplicate script
    const ComponentScriptSlot* origScriptSlot = orig.get_component_script(origRoot);
    if (origScriptSlot)
    {
        ComponentScriptSlot* dupScriptSlot = dup.create_component_script_slot(dupID, origScriptSlot->assetID);
        dupScriptSlot->isEnabled = origScriptSlot->isEnabled;
    }

    bool success = true;

    for (const ComponentBase* child = origBase->child; child; child = child->next)
    {
        // TODO: this traversal reverses sibling order?
        success = success && duplicate_subtree(dup, orig, child->id);
    }

    return success;
}

static Transform* get_transform(void* comp)
{
    return &((TransformComponent*)comp)->transform;
}

static Transform* get_mesh_transform(void* comp)
{
    return &((MeshComponent*)comp)->transform;
}

static AUID get_mesh_auid(void* comp)
{
    return ((MeshComponent*)comp)->auid;
}

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

struct ComponentEntry
{
    ComponentBase* base;
    ComponentScriptSlot* script;
    void* comp;
};

static_assert(LD::IsTrivial<ComponentEntry>);

struct DataRegistryObj
{
    std::unordered_map<ComponentType, PoolAllocator> componentPAs;
    std::unordered_map<CUID, ComponentEntry> components;
    std::unordered_set<CUID> roots;
    PoolAllocator componentBasePA;
    PoolAllocator scriptPA;
    uint32_t idCounter = 0;

    CUID get_id()
    {
        do
        {
            idCounter++; // linear probing for next valid component ID
        } while (idCounter == 0 || components.contains(idCounter));

        return idCounter;
    }

    inline bool is_id_used(CUID id)
    {
        return components.contains(id);
    }

    /// @brief Detach a component from its parent.
    void detach(ComponentBase* base);

    /// @brief Establish a parent-child relationship between components
    void add_child(ComponentBase* parent, ComponentBase* child);

    /// @brief Get component local transform
    Transform* get_component_transform(ComponentBase* base, void* comp);

    /// @brief Mark the local transform of a component subtree as dirty.
    void mark_component_transform_dirty(ComponentBase* base);

    /// @brief Get component world transform matrix
    bool get_component_transform_mat4(ComponentBase* base, Mat4& mat4);
};

void DataRegistryObj::detach(ComponentBase* base)
{
    if (!base || !base->parent)
        return;

    ComponentBase* parent = base->parent;
    ComponentBase** pnext = &(parent->child);
    while (*pnext && *pnext != base)
        pnext = &(*pnext)->next;

    LD_ASSERT(*pnext == base);
    *pnext = base->next;
    base->parent = nullptr;
    roots.insert(base->id);
}

void DataRegistryObj::add_child(ComponentBase* parent, ComponentBase* child)
{
    if (!child->parent && parent)
        roots.erase(child->id);

    child->parent = parent;

    if (parent)
    {
        child->next = parent->child;
        parent->child = child;
    }
}

Transform* DataRegistryObj::get_component_transform(ComponentBase* base, void* comp)
{
    if (!base || !sComponentTable[base->type].get_transform)
        return nullptr;

    return sComponentTable[base->type].get_transform(comp);
}

void DataRegistryObj::mark_component_transform_dirty(ComponentBase* base)
{
    if (!base || (base->flags & COMPONENT_FLAG_TRANSFORM_DIRTY_BIT))
        return;

    base->flags |= COMPONENT_FLAG_TRANSFORM_DIRTY_BIT;

    for (ComponentBase* child = base->child; child; child = child->next)
    {
        mark_component_transform_dirty(child);
    }
}

bool DataRegistryObj::get_component_transform_mat4(ComponentBase* base, Mat4& mat4)
{
    if (!base)
        return false;

    if (base->flags & COMPONENT_FLAG_TRANSFORM_DIRTY_BIT)
    {
        Mat4 parentWorldMat4(1.0f);
        if (base->parent && !get_component_transform_mat4(base->parent, parentWorldMat4))
            return false;

        Transform transform;
        if (!DataRegistry(this).get_component_transform(base->id, transform))
            return false;

        base->localMat4 = transform.as_mat4();
        base->worldMat4 = parentWorldMat4 * base->localMat4;
        base->flags &= ~COMPONENT_FLAG_TRANSFORM_DIRTY_BIT;
    }

    mat4 = base->worldMat4;
    return true;
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

DataRegistry DataRegistry::duplicate() const
{
    LD_PROFILE_SCOPE;

    DataRegistry dup = DataRegistry::create();
    DataRegistry orig(mObj);

    std::vector<CUID> roots;
    orig.get_root_components(roots);

    for (CUID origRoot : roots)
    {
        duplicate_subtree(dup, orig, origRoot);
    }

    return dup;
}

CUID DataRegistry::create_component(ComponentType type, const char* name, CUID parentID, CUID hintID)
{
    if (hintID && mObj->is_id_used(hintID))
    {
        sLog.warn("create_component hint ID {} is in use, failed to create component", hintID);
        return 0;
    }

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
    base->flags = 0;
    base->type = type;
    base->id = hintID ? hintID : mObj->get_id();

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

void DataRegistry::destroy_component(CUID comp)
{
    auto ite = mObj->components.find(comp);

    if (ite == mObj->components.end())
        return;

    ComponentEntry& entry = ite->second;
    LD_ASSERT(entry.base);
    LD_ASSERT(mObj->componentPAs.contains(entry.base->type));
    LD_ASSERT(!entry.script);

    mObj->componentPAs[entry.base->type].free(entry.comp);
    mObj->componentBasePA.free(entry.base);
    mObj->components.erase(ite);
}

void DataRegistry::reparent(CUID compID, CUID parentID)
{
    auto ite = mObj->components.find(compID);
    if (ite == mObj->components.end())
        return;

    ComponentBase* child = ite->second.base;

    ite = mObj->components.find(parentID);
    ComponentBase* parent = ite == mObj->components.end() ? nullptr : ite->second.base;

    mObj->detach(child);
    mObj->add_child(parent, child);
    mObj->mark_component_transform_dirty(child);
}

ComponentScriptSlot* DataRegistry::create_component_script_slot(CUID compID, AUID assetID)
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

void DataRegistry::destroy_component_script_slot(CUID compID)
{
    auto ite = mObj->components.find(compID);
    if (ite == mObj->components.end())
        return;

    ComponentScriptSlot* script = ite->second.script;
    LD_ASSERT(script); // script slot does not exist

    mObj->scriptPA.free(script);

    ite->second.script = nullptr;
}

ComponentBase* DataRegistry::get_component_base(CUID id)
{
    auto ite = mObj->components.find(id);
    if (ite == mObj->components.end())
        return nullptr;

    return ite->second.base;
}

AUID DataRegistry::get_component_auid(CUID compID)
{
    auto ite = mObj->components.find(compID);
    if (ite == mObj->components.end())
        return (RUID)0;

    ComponentType type = ite->second.base->type;

    if (!sComponentTable[type].get_auid)
        return (RUID)0;

    return sComponentTable[type].get_auid(ite->second.comp);
}

void* DataRegistry::get_component(CUID id, ComponentType& type)
{
    auto ite = mObj->components.find(id);
    if (ite == mObj->components.end())
        return nullptr;

    type = ite->second.base->type;
    return ite->second.comp;
}

void DataRegistry::get_root_components(std::vector<CUID>& roots)
{
    roots.resize(mObj->roots.size());

    int i = 0;

    for (CUID compID : mObj->roots)
        roots[i++] = compID;
}

PoolAllocator::Iterator DataRegistry::get_components(ComponentType type)
{
    LD_ASSERT(mObj->componentPAs.contains(type));

    return mObj->componentPAs[type].begin();
}

ComponentScriptSlot* DataRegistry::get_component_script(CUID comp)
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

bool DataRegistry::get_component_transform(CUID compID, Transform& transform)
{
    auto ite = mObj->components.find(compID);
    if (ite == mObj->components.end())
        return false;

    Transform* ptr = mObj->get_component_transform(ite->second.base, ite->second.comp);
    if (!ptr)
        return false;

    transform = *ptr;
    return true;
}

bool DataRegistry::set_component_transform(CUID compID, const Transform& transform)
{
    auto ite = mObj->components.find(compID);
    if (ite == mObj->components.end())
        return false;

    Transform* ptr = mObj->get_component_transform(ite->second.base, ite->second.comp);
    if (!ptr)
        return false;

    *ptr = transform;
    ComponentBase* base = ite->second.base;
    mObj->mark_component_transform_dirty(base);

    return true;
}

bool DataRegistry::mark_component_transform_dirty(CUID compID)
{
    auto ite = mObj->components.find(compID);
    if (ite == mObj->components.end())
        return false;

    ComponentBase* base = ite->second.base;
    mObj->mark_component_transform_dirty(base);

    return true;
}

bool DataRegistry::get_component_transform_mat4(CUID compID, Mat4& mat4)
{
    ComponentBase* base = get_component_base(compID);

    return mObj->get_component_transform_mat4(base, mat4);
}

} // namespace LD