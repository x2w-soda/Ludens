#include <Ludens/DSA/HashMap.h>
#include <Ludens/DSA/HashSet.h>
#include <Ludens/DSA/IDCounter.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/DataRegistry/DataComponent.h>
#include <Ludens/DataRegistry/DataRegistry.h>
#include <Ludens/Header/Types.h>
#include <Ludens/Log/Log.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Profiler/Profiler.h>

namespace LD {

static Log sLog("DataRegistry");
static bool duplicate_subtree(DataRegistry dup, DataRegistry orig, CUID origRoot);
static SUID get_mesh_asset_id(void* comp);

using ComponentTypeFlag = uint32_t;
enum ComponentTypeFlagBit : uint32_t
{
    COMPONENT_TYPE_FLAG_TRANSFORM_EX = LD_BIT(1),
    COMPONENT_TYPE_FLAG_TRANSFORM_2D = LD_BIT(2),
};

struct ComponentMeta
{
    ComponentType type;
    size_t byteSize;
    const char* typeName;
    const ComponentTypeFlag typeFlags;
    SUID (*get_asset_id)(void* compData);
};

// clang-format off
static ComponentMeta sComponentTable[] = {
    { COMPONENT_TYPE_DATA,           sizeof(ComponentBase),          "DataComponent",        0, nullptr },
    { COMPONENT_TYPE_AUDIO_SOURCE,   sizeof(AudioSourceComponent),   "AudioSourceComponent", 0, nullptr },
    { COMPONENT_TYPE_TRANSFORM,      sizeof(TransformComponent),     "TransformComponent",   COMPONENT_TYPE_FLAG_TRANSFORM_EX, nullptr },
    { COMPONENT_TYPE_CAMERA,         sizeof(CameraComponent),        "CameraComponent",      COMPONENT_TYPE_FLAG_TRANSFORM_EX, nullptr },
    { COMPONENT_TYPE_MESH,           sizeof(MeshComponent),          "MeshComponent",        COMPONENT_TYPE_FLAG_TRANSFORM_EX, &get_mesh_asset_id },
    { COMPONENT_TYPE_SPRITE_2D,      sizeof(Sprite2DComponent),      "Sprite2DComponent",    COMPONENT_TYPE_FLAG_TRANSFORM_2D, nullptr },
};
// clang-format on

static bool duplicate_subtree(DataRegistry dup, DataRegistry orig, CUID origRoot)
{
    const ComponentBase* origBase = orig.get_component_base(origRoot);
    const ComponentBase* origParentBase = origBase->parent;
    CUID parentID = origParentBase ? origParentBase->cuid : 0;
    CUID dupID = dup.create_component(origBase->type, origBase->name, parentID, origBase->suid);

    if (dupID != origBase->suid)
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
    void* dupComp = dup.get_component_data(dupID, &type);
    void* origComp = orig.get_component_data(dupID, &type);
    memcpy(dupComp, origComp, sComponentTable[(int)type].byteSize);

    bool success = true;

    for (const ComponentBase* child = origBase->child; child; child = child->next)
    {
        // TODO: this traversal reverses sibling order?
        success = success && duplicate_subtree(dup, orig, child->suid);
    }

    return success;
}

static SUID get_mesh_asset_id(void* comp)
{
    return ((MeshComponent*)comp)->assetID;
}

static_assert(sizeof(sComponentTable) / sizeof(*sComponentTable) == COMPONENT_TYPE_ENUM_COUNT);
static_assert(LD::IsDataComponent<AudioSourceComponent>);
static_assert(LD::IsDataComponent<TransformComponent>);
static_assert(LD::IsDataComponent<CameraComponent>);
static_assert(LD::IsDataComponent<MeshComponent>);
static_assert(LD::IsDataComponent<Sprite2DComponent>);

size_t get_component_byte_size(ComponentType type)
{
    return sComponentTable[(int)type].byteSize;
}

const char* get_component_type_name(ComponentType type)
{
    return sComponentTable[(int)type].typeName;
}

struct DataRegistryObj
{
    HashMap<ComponentType, PoolAllocator> componentPAs;
    HashMap<CUID, ComponentBase**> cuidToCompData;
    HashMap<SUID, ComponentBase**> suidToCompData;
    HashSet<CUID> roots;
    PoolAllocator componentBasePA;
    IDCounter<CUID> cuidCounter;

    inline ComponentBase** get_data_from_cuid(CUID compCUID)
    {
        auto it = cuidToCompData.find(compCUID);

        if (it != cuidToCompData.end())
        {
            LD_ASSERT(it->second && *it->second);
            return (ComponentBase**)it->second;
        }

        return nullptr;
    }

    inline ComponentBase** get_data_from_suid(SUID compSUID)
    {
        auto it = suidToCompData.find(compSUID);

        if (it != suidToCompData.end())
        {
            LD_ASSERT(it->second && *it->second);
            return (ComponentBase**)it->second;
        }

        return nullptr;
    }

    /// @brief Detach a component from its parent.
    void detach(ComponentBase* base);

    /// @brief Establish a parent-child relationship between components
    void add_child(ComponentBase* parent, ComponentBase* child);

    /// @brief Get component local transform.
    inline TransformEx* get_component_transform(ComponentBase** data)
    {
        LD_ASSERT(data);
        ComponentBase* base = *data;

        if (!base || !(sComponentTable[base->type].typeFlags & COMPONENT_TYPE_FLAG_TRANSFORM_EX))
            return nullptr;

        return (TransformEx*)(data + 1);
    }

    /// @brief Get component local 2D transform.
    inline Transform2D* get_component_transform_2d(ComponentBase** data)
    {
        LD_ASSERT(data);
        ComponentBase* base = *data;

        if (!base || !(sComponentTable[base->type].typeFlags & COMPONENT_TYPE_FLAG_TRANSFORM_2D))
            return nullptr;

        return (Transform2D*)(data + 1);
    }

    /// @brief Mark the local transform of a component subtree as dirty.
    void mark_component_transform_dirty(ComponentBase* base);

    /// @brief Get component world transform matrix
    bool get_component_world_mat4(ComponentBase* base, Mat4& mat4);
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
    roots.insert(base->suid);
}

void DataRegistryObj::add_child(ComponentBase* parent, ComponentBase* child)
{
    if (!child->parent && parent)
        roots.erase(child->suid);

    child->parent = parent;

    if (parent)
    {
        child->next = parent->child;
        parent->child = child;
    }
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

bool DataRegistryObj::get_component_world_mat4(ComponentBase* base, Mat4& mat4)
{
    if (!base)
        return false;

    if (base->flags & COMPONENT_FLAG_TRANSFORM_DIRTY_BIT)
    {
        Mat4 parentWorldMat4(1.0f);
        if (base->parent && !get_component_world_mat4(base->parent, parentWorldMat4))
            return false;

        TransformEx transform;
        Transform2D transform2D;
        if (DataRegistry(this).get_component_transform(base->cuid, transform))
        {
            transform.rotation = Quat::from_euler(transform.rotationEuler);
            base->localMat4 = transform.as_mat4();
        }
        else if (DataRegistry(this).get_component_transform_2d(base->cuid, transform2D))
        {
            base->localMat4 = transform2D.as_mat4();
        }
        else
            return false;

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
    paI.pageSize = 256;
    paI.isMultiPage = true;
    paI.usage = MEMORY_USAGE_MISC;
    obj->componentBasePA = PoolAllocator::create(paI);

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

CUID DataRegistry::create_component(ComponentType type, const char* name, CUID parentID, SUID hintSUID)
{
    if (hintSUID && !try_get_suid(hintSUID))
    {
        sLog.warn("create_component hint SUID {} is in use, failed to create component", hintSUID);
        return 0;
    }

    size_t compDataByteSize = get_component_byte_size(type);

    if (!mObj->componentPAs.contains(type))
    {
        PoolAllocatorInfo paI{};
        paI.blockSize = compDataByteSize;
        paI.pageSize = 1024;
        paI.isMultiPage = true;
        paI.usage = MEMORY_USAGE_MISC;
        mObj->componentPAs[type] = PoolAllocator::create(paI);
    }

    // allocate base members
    ComponentBase* compBase = (ComponentBase*)mObj->componentBasePA.allocate();
    memset(compBase, 0, sizeof(ComponentBase));

    compBase->name = heap_strdup(name, MEMORY_USAGE_MISC);
    compBase->type = type;
    compBase->suid = hintSUID;                   // serial identity, may be zero for components created at runtime
    compBase->cuid = mObj->cuidCounter.get_id(); // runtime identity

    if (parentID)
    {
        ComponentBase* parent = *(mObj->cuidToCompData[parentID]);
        mObj->add_child(parent, compBase);
    }
    else
    {
        mObj->roots.insert(compBase->cuid);
    }

    // allocate component type
    void* compData = mObj->componentPAs[type].allocate();
    memset(compData, 0, compDataByteSize);

    // first member of component data is backwards link to it's ComponentBase metadata
    *(ComponentBase**)compData = compBase;

    mObj->cuidToCompData[compBase->cuid] = (ComponentBase**)compData;
    if (compBase->suid)
        mObj->suidToCompData[compBase->suid] = (ComponentBase**)compData;

    return compBase->cuid;
}

void DataRegistry::destroy_component(CUID compCUID)
{
    auto it = mObj->cuidToCompData.find(compCUID);

    if (it == mObj->cuidToCompData.end())
        return;

    ComponentBase** compData = it->second;
    LD_ASSERT(compData && *compData);

    ComponentBase* compBase = *compData;
    LD_ASSERT(mObj->componentPAs.contains(compBase->type));

    SUID compSUID = compBase->suid;

    mObj->componentPAs[compBase->type].free(compData);
    mObj->componentBasePA.free(compBase);

    mObj->cuidToCompData.erase(compCUID);
    mObj->suidToCompData.erase(compSUID);
}

void DataRegistry::reparent(CUID compID, CUID parentID)
{
    ComponentBase** childData = mObj->get_data_from_cuid(compID);
    ComponentBase** parentData = mObj->get_data_from_cuid(parentID);
    if (!childData || !parentData)
        return;

    ComponentBase* childBase = *childData;
    ComponentBase* parentBase = *parentData;

    mObj->detach(childBase);
    mObj->add_child(parentBase, childBase);
    mObj->mark_component_transform_dirty(childBase);
}

ComponentBase* DataRegistry::get_component_base(CUID compCUID)
{
    ComponentBase** data = mObj->get_data_from_cuid(compCUID);

    return data ? *data : nullptr;
}

SUID DataRegistry::get_component_asset_id(CUID compID)
{
    auto it = mObj->cuidToCompData.find(compID);
    if (it == mObj->cuidToCompData.end())
        return (SUID)0;

    ComponentBase** compData = it->second;
    LD_ASSERT(compData);
    ComponentBase* compBase = *compData;

    if (!sComponentTable[compBase->type].get_asset_id)
        return (SUID)0;

    return sComponentTable[compBase->type].get_asset_id(compData);
}

ComponentBase** DataRegistry::get_component_data(CUID compID, ComponentType* outType)
{
    ComponentBase** compData = mObj->get_data_from_cuid(compID);
    if (!compData)
        return nullptr;

    ComponentBase* compBase = *compData;
    if (outType)
        *outType = compBase->type;

    return compData;
}

ComponentBase** DataRegistry::get_component_data_by_suid(SUID compSUID, ComponentType* outType)
{
    ComponentBase** compData = mObj->get_data_from_suid(compSUID);
    if (!compData)
        return nullptr;

    ComponentBase* compBase = *compData;
    if (outType)
        *outType = compBase->type;

    return compData;
}

void DataRegistry::get_root_components(Vector<CUID>& roots)
{
    roots.resize(mObj->roots.size());

    int i = 0;

    for (CUID compID : mObj->roots)
        roots[i++] = compID;
}

PoolAllocator::Iterator DataRegistry::get_components(ComponentType type)
{
    auto ite = mObj->componentPAs.find(type);
    if (ite == mObj->componentPAs.end())
        return {nullptr, nullptr, 0};

    return ite->second.begin();
}

bool DataRegistry::get_component_transform(CUID compID, TransformEx& transform)
{
    ComponentBase** data = mObj->get_data_from_cuid(compID);
    if (!data)
        return false;

    TransformEx* srcTransform = mObj->get_component_transform(data);
    if (!srcTransform)
        return false;

    transform = *srcTransform;
    return true;
}

bool DataRegistry::get_component_transform_2d(CUID compID, Transform2D& transform)
{
    ComponentBase** data = mObj->get_data_from_cuid(compID);
    if (!data)
        return false;

    Transform2D* srcTransform = mObj->get_component_transform_2d(data);
    if (!srcTransform)
        return false;

    transform = *srcTransform;
    return true;
}

bool DataRegistry::set_component_transform(CUID compID, const TransformEx& transform)
{
    ComponentBase** data = mObj->get_data_from_cuid(compID);
    if (!data)
        return false;

    TransformEx* dstTransform = mObj->get_component_transform(data);
    if (!dstTransform)
        return false;

    *dstTransform = transform;
    mObj->mark_component_transform_dirty(*data);

    return true;
}

bool DataRegistry::set_component_transform_2d(CUID compID, const Transform2D& transform)
{
    ComponentBase** data = mObj->get_data_from_cuid(compID);
    if (!data)
        return false;

    Transform2D* dstTransform = mObj->get_component_transform_2d(data);
    if (!dstTransform)
        return false;

    *dstTransform = transform;
    mObj->mark_component_transform_dirty(*data);

    return true;
}

bool DataRegistry::mark_component_transform_dirty(CUID compID)
{
    ComponentBase** data = mObj->get_data_from_cuid(compID);
    if (!data)
        return false;

    mObj->mark_component_transform_dirty(*data);

    return true;
}

bool DataRegistry::get_component_world_mat4(CUID compID, Mat4& mat4)
{
    LD_PROFILE_SCOPE;

    ComponentBase* base = get_component_base(compID);

    return mObj->get_component_world_mat4(base, mat4);
}

} // namespace LD