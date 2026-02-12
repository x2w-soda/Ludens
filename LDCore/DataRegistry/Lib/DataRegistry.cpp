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
static IDCounter<CUID> sCUIDCounter;
static bool duplicate_subtree(DataRegistry dst, CUID dstParentID, DataRegistry src, CUID srcID);
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

struct DataRegistryObj
{
    HashMap<ComponentType, PoolAllocator> componentPAs;
    HashMap<CUID, ComponentBase**> cuidToCompData;
    HashMap<SUID, ComponentBase**> suidToCompData;
    HashSet<CUID> roots; // TODO: root should be ordered
    PoolAllocator componentBasePA;

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

    /// @brief Mark the local transform of a component subtree as dirty.
    void mark_component_transform_dirty(ComponentBase* base);

    /// @brief Get component world transform matrix
    bool get_component_world_mat4(ComponentBase* base, Mat4& mat4);
};

static bool duplicate_subtree(DataRegistry dst, CUID dstParentID, DataRegistry src, CUID srcID)
{
    const ComponentBase* srcBase = src.get_component_base(srcID);
    LD_ASSERT(srcBase);

    CUID dstID = dst.create_component(srcBase->type, srcBase->name, dstParentID, (SUID)0);

    if (!dstID)
    {
        sLog.error("failed to duplicate {}", srcBase->name);
        return false;
    }

    // copy base fields, note that serial ID is copied over.
    ComponentBase** dstData = dst.get_component_data(dstID, nullptr);
    LD_ASSERT(dstData);

    ComponentBase* dstBase = *dstData;
    dstBase->suid = srcBase->suid;
    dstBase->scriptAssetID = srcBase->scriptAssetID;
    dstBase->localMat4 = srcBase->localMat4;
    dstBase->worldMat4 = srcBase->worldMat4;
    dst.unwrap()->suidToCompData[dstBase->suid] = dstData;

    // copy transform state
    if (sComponentTable[(int)srcBase->type].typeFlags & COMPONENT_TYPE_FLAG_TRANSFORM_2D)
    {
        ComponentBase** srcData = dst.get_component_data(srcID, nullptr);
        Transform2D* dstTransform = get_component_transform_2d(dstData);
        Transform2D* srcTransform = get_component_transform_2d(srcData);
        LD_ASSERT(srcTransform && dstTransform);
        *dstTransform = *srcTransform;
        dstBase->flags |= COMPONENT_FLAG_TRANSFORM_DIRTY_BIT;
    }
    else if (sComponentTable[(int)srcBase->type].typeFlags & COMPONENT_TYPE_FLAG_TRANSFORM_EX)
    {
        ComponentBase** srcData = src.get_component_data(srcID, nullptr);
        TransformEx* dstTransform = get_component_transform(dstData);
        TransformEx* srcTransform = get_component_transform(srcData);
        LD_ASSERT(srcTransform && dstTransform);
        *dstTransform = *srcTransform;
        dstBase->flags |= COMPONENT_FLAG_TRANSFORM_DIRTY_BIT;
    }

    // maintain sibling order
    Vector<const ComponentBase*> srcChildOrder;
    for (const ComponentBase* srcChild = srcBase->child; srcChild; srcChild = srcChild->next)
        srcChildOrder.push_back(srcChild);

    for (auto it = srcChildOrder.rbegin(); it != srcChildOrder.rend(); ++it)
    {
        const ComponentBase* srcChild = *it;

        if (!duplicate_subtree(dst, dstID, src, srcChild->cuid))
            return false;
    }

    return true;
}

static SUID get_mesh_asset_id(void* comp)
{
    return ((MeshComponent*)comp)->assetID;
}

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
    roots.insert(base->cuid);
}

void DataRegistryObj::add_child(ComponentBase* parent, ComponentBase* child)
{
    if (!child->parent && parent)
        roots.erase(child->cuid);

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

    DataRegistry dst = DataRegistry::create();
    DataRegistry src(mObj);

    for (CUID srcRoot : mObj->roots)
    {
        bool ok = duplicate_subtree(dst, 0, src, srcRoot);
        LD_ASSERT(ok);
    }

    return dst;
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
    compBase->suid = hintSUID;              // serial identity, may be zero for components created at runtime
    compBase->cuid = sCUIDCounter.get_id(); // runtime identity

    if (parentID)
    {
        LD_ASSERT(mObj->cuidToCompData.contains(parentID));
        ComponentBase* parentBase = *(mObj->cuidToCompData[parentID]);
        mObj->add_child(parentBase, compBase);
    }
    else
    {
        mObj->roots.insert(compBase->cuid);
    }

    // allocate component type
    ComponentBase** compData = (ComponentBase**)mObj->componentPAs[type].allocate();
    memset(compData, 0, compDataByteSize);

    // first member of component data is backwards link to it's ComponentBase metadata
    *compData = compBase;

    mObj->cuidToCompData[compBase->cuid] = compData;
    if (compBase->suid)
        mObj->suidToCompData[compBase->suid] = compData;

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

    *compData = nullptr;
    mObj->componentPAs[compBase->type].free(compData);

    compBase->type = COMPONENT_TYPE_DATA;
    compBase->cuid = 0;
    compBase->suid = 0;
    compBase->flags = 0;
    compBase->scriptAssetID = 0;
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
    LD_PROFILE_SCOPE;

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
    LD_PROFILE_SCOPE;

    ComponentBase** compData = mObj->get_data_from_suid(compSUID);
    if (!compData)
        return nullptr;

    ComponentBase* compBase = *compData;
    if (outType)
        *outType = compBase->type;

    return compData;
}

void DataRegistry::get_root_component_data(Vector<ComponentBase**>& rootData)
{
    LD_PROFILE_SCOPE;

    rootData.resize(mObj->roots.size());

    int i = 0;

    for (CUID compID : mObj->roots)
    {
        ComponentBase** data = get_component_data(compID, nullptr);
        LD_ASSERT(data);
        rootData[i++] = data;
    }
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
    LD_PROFILE_SCOPE;

    ComponentBase** data = mObj->get_data_from_cuid(compID);
    if (!data)
        return false;

    TransformEx* srcTransform = LD::get_component_transform(data);
    if (!srcTransform)
        return false;

    transform = *srcTransform;
    return true;
}

bool DataRegistry::get_component_transform_2d(CUID compID, Transform2D& transform)
{
    LD_PROFILE_SCOPE;

    ComponentBase** data = mObj->get_data_from_cuid(compID);
    if (!data)
        return false;

    Transform2D* srcTransform = LD::get_component_transform_2d(data);
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

    TransformEx* dstTransform = LD::get_component_transform(data);
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

    Transform2D* dstTransform = LD::get_component_transform_2d(data);
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