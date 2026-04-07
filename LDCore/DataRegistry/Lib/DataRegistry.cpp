#include <Ludens/DSA/HashMap.h>
#include <Ludens/DSA/HashSet.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/DataRegistry/DataComponent.h>
#include <Ludens/DataRegistry/DataRegistry.h>
#include <Ludens/DataRegistry/TransformRegistry.h>
#include <Ludens/Header/Types.h>
#include <Ludens/Log/Log.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Profiler/Profiler.h>

#define COMPONENT_TYPE_TRANSFORM_BITS (COMPONENT_TYPE_FLAG_TRANSFORM_2D | COMPONENT_TYPE_FLAG_TRANSFORM_EX)

namespace LD {

static Log sLog("DataRegistry");
static IDRegistry sCUIDRegistry;
static ComponentBase** duplicate_subtree(DataRegistry dst, CUID dstParentID, SUIDRegistry dstSUIDRegistry, DataRegistry src, CUID srcID);
static ComponentBase** duplicate_component(ComponentBase** srcData, CUID dstParentID, DataRegistry dstRegistry, SUID dstSUID);
static SUID get_mesh_asset_id(void* comp);

enum ComponentPlacement
{
    COMPONENT_PLACEMENT_AS_FIRST_CHILD,
    COMPONENT_PLACEMENT_AS_LAST_CHILD,
    COMPONENT_PLACEMENT_AFTER_SIBLING,
};

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
    { COMPONENT_TYPE_TRANSFORM_2D,   sizeof(Transform2DComponent),   "Transform2DComponent", COMPONENT_TYPE_FLAG_TRANSFORM_2D, nullptr },
    { COMPONENT_TYPE_CAMERA,         sizeof(CameraComponent),        "CameraComponent",      COMPONENT_TYPE_FLAG_TRANSFORM_EX, nullptr },
    { COMPONENT_TYPE_CAMERA_2D,      sizeof(Camera2DComponent),      "Camera2DComponent",    COMPONENT_TYPE_FLAG_TRANSFORM_2D, nullptr },
    { COMPONENT_TYPE_MESH,           sizeof(MeshComponent),          "MeshComponent",        COMPONENT_TYPE_FLAG_TRANSFORM_EX, &get_mesh_asset_id },
    { COMPONENT_TYPE_SPRITE_2D,      sizeof(Sprite2DComponent),      "Sprite2DComponent",    COMPONENT_TYPE_FLAG_TRANSFORM_2D, nullptr },
    { COMPONENT_TYPE_SCREEN_UI,      sizeof(ScreenUIComponent),      "ScreenUIComponent",    0, nullptr },
};
// clang-format on

static_assert(sizeof(sComponentTable) / sizeof(*sComponentTable) == COMPONENT_TYPE_ENUM_COUNT);
static_assert(LD::IsDataComponent<AudioSourceComponent>);
static_assert(LD::IsDataComponent<TransformComponent>);
static_assert(LD::IsDataComponent<Transform2DComponent>);
static_assert(LD::IsDataComponent<CameraComponent>);
static_assert(LD::IsDataComponent<Camera2DComponent>);
static_assert(LD::IsDataComponent<MeshComponent>);
static_assert(LD::IsDataComponent<Sprite2DComponent>);
static_assert(LD::IsDataComponent<ScreenUIComponent>);

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

    LD_ASSERT(base->transformEx);
    return base->transformEx;
}

/// @brief Get component local 2D transform.
inline Transform2D* get_component_transform_2d(ComponentBase** data)
{
    LD_ASSERT(data);
    ComponentBase* base = *data;

    if (!base || !(sComponentTable[base->type].typeFlags & COMPONENT_TYPE_FLAG_TRANSFORM_2D))
        return nullptr;

    LD_ASSERT(base->transform2D);
    return base->transform2D;
}

struct DataRegistryObj
{
    HashMap<ComponentType, PoolAllocator> componentPAs;
    Vector<ComponentBase**> cuidToCompData;
    HashMap<SUID, ComponentBase**> suidToCompData;
    ComponentBase root;
    Vector<CUID> topLevel;
    PoolAllocator componentBasePA;
    Transform2DRegistry transform2DRegistry;

    inline ComponentBase** get_data_from_cuid(CUID compCUID)
    {
        uint32_t compIndex = compCUID.index();

        return compCUID && (compIndex < cuidToCompData.size()) ? cuidToCompData[compIndex] : nullptr;
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

    /// @brief Sparse index is used for O(1) array indexing,
    ///        we need to grow vectors to accommodate max sparse index.
    void reserve_sparse_index(CUID id);

    /// @brief Detach a component from its parent.
    void detach(ComponentBase* base);

    /// @brief Establish a parent-child relationship between components
    void add_child(ComponentBase* child, ComponentBase* parent, ComponentBase* placementTarget, ComponentPlacement placement);

    /// @brief Get component world transform matrix
    bool get_component_world_mat4(ComponentBase* base, Mat4& mat4);

    bool get_component_world_transform_2d(ComponentBase* base, Transform2D& transform2D);

    void destroy_subtree(ComponentBase** data);
    void destroy_component(ComponentBase** data);
    ComponentBase** clone_subtree(ComponentBase** srcData, SUIDRegistry suidRegistry, ComponentPlacement placement);
    void print_subtree(ComponentBase** data, std::string& str, int indent);

    static void id_hierarchy(ID parent, Vector<ID>& children, void* user);
};

static ComponentBase** duplicate_component(ComponentBase** srcData, CUID dstParentID, DataRegistry dstRegistry, SUID dstSUID)
{
    ComponentBase* srcBase = *srcData;
    CUID dstID = dstRegistry.create_component(srcBase->type, srcBase->name, dstParentID, dstSUID);

    if (!dstID)
        return nullptr;

    ComponentBase** dstData = dstRegistry.get_component_data(dstID, nullptr);
    ComponentBase* dstBase = *dstData;
    dstBase->scriptAssetID = srcBase->scriptAssetID;

    // deep copy transform state
    if (sComponentTable[(int)srcBase->type].typeFlags & COMPONENT_TYPE_FLAG_TRANSFORM_2D)
    {
        Transform2D* dstTransform = dstBase->transform2D;
        Transform2D* srcTransform = srcBase->transform2D;
        LD_ASSERT(srcTransform && dstTransform);
        *dstTransform = *srcTransform;
    }
    else if (sComponentTable[(int)srcBase->type].typeFlags & COMPONENT_TYPE_FLAG_TRANSFORM_EX)
    {
        TransformEx* dstTransform = dstBase->transformEx;
        TransformEx* srcTransform = srcBase->transformEx;
        LD_ASSERT(srcTransform && dstTransform);
        *dstTransform = *srcTransform;
    }

    return dstData;
}

// Makes a deep copy of src component subtree from src registry into dst registry.
// Src and dst registries may or may not be the same.
static ComponentBase** duplicate_subtree(DataRegistry dst, CUID dstParentID, SUIDRegistry suidRegistry, DataRegistry src, CUID srcID)
{
    ComponentBase** srcData = src.get_component_data(srcID, nullptr);
    LD_ASSERT(srcData);

    ComponentBase* srcBase = *srcData;

    // Play-in-editor may wish to copy the SUID over from src to dst.
    // Copy-pasting a subtree in editor will have to generate new SUIDs.
    SUID dstSUID = srcBase->suid;
    if (suidRegistry)
        dstSUID = suidRegistry.get_suid(SERIAL_TYPE_COMPONENT);

    ComponentBase** dstData = duplicate_component(srcData, dstParentID, dst, dstSUID);
    if (!dstData)
    {
        sLog.error("failed to duplicate {}", srcBase->name);
        return nullptr;
    }

    CUID dstID = (*dstData)->cuid;

    for (const ComponentBase* srcChild = srcBase->child; srcChild; srcChild = srcChild->next)
    {
        if (!duplicate_subtree(dst, dstID, suidRegistry, src, srcChild->cuid))
            return nullptr;
    }

    return dstData;
}

static SUID get_mesh_asset_id(void* comp)
{
    return ((MeshComponent*)comp)->assetID;
}

void DataRegistryObj::reserve_sparse_index(CUID id)
{
    if (!id)
        return;

    uint32_t sparseIndex = id.index();

    if (sparseIndex >= cuidToCompData.size())
    {
        cuidToCompData.resize(sparseIndex + 1);
        for (uint32_t i = sparseIndex; i < cuidToCompData.size(); i++)
            cuidToCompData[i] = nullptr;
    }
}

void DataRegistryObj::detach(ComponentBase* base)
{
    if (!base)
        return;

    LD_ASSERT(base->parent); // already detached

    ComponentBase* parent = base->parent;

    if (parent->child == base)
    {
        parent->child = base->next;
        base->parent = nullptr;
        base->next = nullptr;
        return;
    }

    ComponentBase* pos = parent->child;
    while (pos && pos->next != base)
        pos = pos->next;

    LD_ASSERT(pos);
    pos->next = base->next;
    base->parent = nullptr;
    base->next = nullptr;
}

void DataRegistryObj::add_child(ComponentBase* child, ComponentBase* parent, ComponentBase* placementTarget, ComponentPlacement placement)
{
    LD_ASSERT(parent);
    LD_ASSERT(child && !child->parent && !child->next);                 // not detached
    LD_ASSERT(!(placementTarget && placementTarget->parent != parent)); // hello?

    child->parent = parent;

    if (!parent->child)
    {
        parent->child = child;
        return;
    }

    ComponentBase* pos = parent->child;

    switch (placement)
    {
    case COMPONENT_PLACEMENT_AS_FIRST_CHILD:
        child->next = parent->child;
        parent->child = child;
        break;
    case COMPONENT_PLACEMENT_AS_LAST_CHILD:
        while (pos && pos->next)
            pos = pos->next;
        LD_ASSERT(pos);
        pos->next = child;
        break;
    case COMPONENT_PLACEMENT_AFTER_SIBLING:
        LD_ASSERT(placementTarget);
        while (pos && pos != placementTarget)
            pos = pos->next;
        LD_ASSERT(pos);
        child->next = pos->next;
        pos->next = child;
        break;
    default:
        LD_UNREACHABLE;
    }
}

bool DataRegistryObj::get_component_world_mat4(ComponentBase* base, Mat4& mat4)
{
    if (!base || !(sComponentTable[(int)base->type].typeFlags & COMPONENT_TYPE_TRANSFORM_BITS))
        return false;

    mat4 = transform2DRegistry.get_world_mat4(base->cuid);
    return true;
}

bool DataRegistryObj::get_component_world_transform_2d(ComponentBase* base, Transform2D& transform)
{
    if (!base || !(sComponentTable[(int)base->type].typeFlags & COMPONENT_TYPE_FLAG_TRANSFORM_2D))
        return false;

    return transform2DRegistry.get_world_transform(base->cuid, transform);
}

void DataRegistryObj::destroy_subtree(ComponentBase** compData)
{
    LD_ASSERT(compData && *compData);

    ComponentBase* compBase = *compData;

    for (ComponentBase* child = compBase->child; child; child = child->next)
    {
        ComponentBase** childData = cuidToCompData[child->cuid.index()];
        destroy_subtree(childData);
    }

    destroy_component(compData);
}

// NOTE: The parent's child linked list is not updated here.
void DataRegistryObj::destroy_component(ComponentBase** compData)
{
    LD_ASSERT(compData && *compData);

    ComponentBase* compBase = *compData;
    SUID compSUID = compBase->suid;
    CUID compCUID = compBase->cuid;
    ComponentType compType = compBase->type;
    LD_ASSERT(componentPAs.contains(compType));

    *compData = nullptr;
    componentPAs[compType].free(compData);

    compBase->type = COMPONENT_TYPE_DATA;
    compBase->cuid = 0;
    compBase->suid = 0;
    compBase->scriptAssetID = 0;

    componentBasePA.free(compBase);

    sCUIDRegistry.destroy(compCUID);

    cuidToCompData[compCUID.index()] = nullptr;
    suidToCompData.erase(compSUID);
}

ComponentBase** DataRegistryObj::clone_subtree(ComponentBase** srcData, SUIDRegistry suidRegistry, ComponentPlacement placement)
{
    DataRegistry reg(this);
    ComponentBase* srcBase = *srcData;
    ComponentBase* srcParent = srcBase->parent;
    LD_ASSERT(srcParent && suidRegistry);

    // duplicate a subtree in the same registry and generate new SUIDs.
    ComponentBase** dstData = duplicate_subtree(reg, srcParent->cuid, suidRegistry, reg, srcBase->cuid);
    if (!dstData)
        return nullptr;

    detach(*dstData);
    add_child(*dstData, srcBase->parent, srcBase, placement);

    return dstData;
}

void DataRegistryObj::print_subtree(ComponentBase** data, std::string& str, int indent)
{
    ComponentBase* base = *data;
    str += std::string(indent, ' ');
    str += std::format("{:10} [CUID {}], [SUID {}]\n", base->name, base->cuid, base->suid);

    for (ComponentBase* child = base->child; child; child = child->next)
    {
        ComponentBase** childData = get_data_from_cuid(child->cuid);
        LD_ASSERT(childData);
        print_subtree(childData, str, indent + 1);
    }
}

void DataRegistryObj::id_hierarchy(ID parent, Vector<ID>& children, void* user)
{
    LD_ASSERT(children.empty());

    DataRegistryObj* obj = (DataRegistryObj*)user;
    ComponentBase** data = obj->get_data_from_cuid(parent);
    if (!data)
        return;

    LD_ASSERT(*data);
    for (ComponentBase* child = (*data)->child; child; child = child->next)
    {
        if (sComponentTable[(int)child->type].typeFlags & COMPONENT_TYPE_TRANSFORM_BITS)
            children.push_back(child->cuid);
    }
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
    obj->root = {}; // root dummy has CUID and SUID of zero.

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

    for (ComponentBase* child = mObj->root.child; child; child = child->next)
    {
        bool ok = duplicate_subtree(dst, 0, {}, src, child->cuid);
        LD_ASSERT(ok);
    }

    return dst;
}

CUID DataRegistry::create_component(ComponentType type, const char* name, CUID parentID, SUID suid)
{
    LD_PROFILE_SCOPE;

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
    compBase->suid = suid;                   // serial identity, may be zero for components created at runtime
    compBase->cuid = sCUIDRegistry.create(); // runtime identity

    if (sComponentTable[(int)type].typeFlags & COMPONENT_TYPE_FLAG_TRANSFORM_2D)
    {
        compBase->transform2D = mObj->transform2DRegistry.create(compBase->cuid, parentID);
    }
    else if (sComponentTable[(int)type].typeFlags & COMPONENT_TYPE_FLAG_TRANSFORM_EX)
    {
        // TODO: TransformRegistry for 3D
        LD_UNREACHABLE;
    }

    mObj->reserve_sparse_index(compBase->cuid);

    ComponentBase* parentBase = &mObj->root;

    if (parentID)
    {
        uint32_t parentIndex = parentID.index();
        LD_ASSERT(mObj->cuidToCompData[parentIndex]);
        parentBase = *(mObj->cuidToCompData[parentIndex]);
    }

    mObj->add_child(compBase, parentBase, nullptr, COMPONENT_PLACEMENT_AS_LAST_CHILD);

    // allocate component type
    ComponentBase** compData = (ComponentBase**)mObj->componentPAs[type].allocate();
    memset(compData, 0, compDataByteSize);

    // first member of component data is backwards link to it's ComponentBase metadata
    *compData = compBase;

    uint32_t compIndex = compBase->cuid.index();
    mObj->cuidToCompData[compIndex] = compData;
    if (compBase->suid)
        mObj->suidToCompData[compBase->suid] = compData;

    return compBase->cuid;
}

void DataRegistry::destroy_component_subtree(CUID compCUID)
{
    LD_PROFILE_SCOPE;

    uint32_t compIndex = compCUID.index();
    ComponentBase** compData = mObj->cuidToCompData[compIndex];

    if (!compData)
        return;

    ComponentBase* compBase = *compData;

    if (sComponentTable[(int)compBase->type].typeFlags & COMPONENT_TYPE_FLAG_TRANSFORM_2D)
    {
        mObj->transform2DRegistry.destroy_subtree(compCUID, &DataRegistryObj::id_hierarchy, mObj);
    }

    mObj->detach(compBase);
    mObj->destroy_subtree(compData);
}

void DataRegistry::reparent_component_subtree(CUID compID, CUID parentID)
{
    ComponentBase** childData = mObj->get_data_from_cuid(compID);
    ComponentBase** parentData = mObj->get_data_from_cuid(parentID);
    if (!childData || !parentData)
        return;

    ComponentBase* childBase = *childData;
    ComponentBase* parentBase = *parentData;

    mObj->detach(childBase);
    mObj->add_child(childBase, parentBase, nullptr, COMPONENT_PLACEMENT_AS_LAST_CHILD);

    mObj->transform2DRegistry.reparent_subtree(compID, parentID, &DataRegistryObj::id_hierarchy, mObj);
}

ComponentBase** DataRegistry::clone_component_subtree(CUID rootID, SUIDRegistry suidRegistry)
{
    ComponentBase** srcData = mObj->get_data_from_cuid(rootID);
    if (!srcData)
        return nullptr;

    ComponentBase** dstData = mObj->clone_subtree(srcData, suidRegistry, COMPONENT_PLACEMENT_AFTER_SIBLING);
    if (!dstData)
        return nullptr;

    return dstData;
}

ComponentBase* DataRegistry::get_component_base(CUID compCUID)
{
    ComponentBase** data = mObj->get_data_from_cuid(compCUID);

    return data ? *data : nullptr;
}

SUID DataRegistry::get_component_asset_id(CUID compID)
{
    uint32_t compIndex = compID.index();
    ComponentBase** compData = mObj->cuidToCompData[compIndex];

    if (!compData)
        return (SUID)0;

    ComponentBase* compBase = *compData;
    LD_ASSERT(compBase);

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
    LD_PROFILE_SCOPE;

    ComponentBase** compData = mObj->get_data_from_suid(compSUID);
    if (!compData)
        return nullptr;

    ComponentBase* compBase = *compData;
    if (outType)
        *outType = compBase->type;

    return compData;
}

ComponentBase** DataRegistry::get_component_data_by_path(const Vector<int>& path)
{
    if (path.empty())
        return nullptr;

    ComponentBase* base = &mObj->root;

    for (int siblingIndex : path)
    {
        ComponentBase* child = base->child;

        for (int i = 0; i < siblingIndex; i++)
        {
            if (!child) // bad sibling index
                return nullptr;

            child = child->next;
        }

        if (!child) // bad sibling index
            return nullptr;

        base = child;
    }

    return mObj->get_data_from_cuid(base->cuid);
}

void DataRegistry::get_root_component_data(Vector<ComponentBase**>& rootData)
{
    LD_PROFILE_SCOPE;

    rootData.clear();

    for (ComponentBase* child = mObj->root.child; child; child = child->next)
    {
        ComponentBase** data = get_component_data(child->cuid, nullptr);
        LD_ASSERT(data);
        rootData.push_back(data);
    }
}

PoolAllocator::Iterator DataRegistry::get_components(ComponentType type)
{
    auto ite = mObj->componentPAs.find(type);
    if (ite == mObj->componentPAs.end())
        return {nullptr, nullptr, 0};

    return ite->second.begin();
}

bool DataRegistry::get_component_path(CUID compID, Vector<int>& path)
{
    ComponentBase** data = mObj->get_data_from_cuid(compID);
    if (!data)
        return false;

    path.clear();

    for (ComponentBase* base = *data; base != &mObj->root; base = base->parent)
    {
        int siblingIndex = 0;
        for (ComponentBase* sib = base->parent->child; sib != base; sib = sib->next)
            siblingIndex++;

        path.push_back(siblingIndex);
    }

    std::reverse(path.begin(), path.end());
    return true;
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

    return true;
}

bool DataRegistry::get_component_world_mat4(CUID compID, Mat4& mat4)
{
    ComponentBase* base = get_component_base(compID);

    return mObj->get_component_world_mat4(base, mat4);
}

bool DataRegistry::get_component_world_transform_2d(CUID compID, Transform2D& transform)
{
    ComponentBase* base = get_component_base(compID);

    return mObj->get_component_world_transform_2d(base, transform);
}

void DataRegistry::invalidate_transforms()
{
    LD_PROFILE_SCOPE;

    mObj->transform2DRegistry.invalidate_transforms();
}

std::string DataRegistry::print_hierarchy()
{
    std::string str;

    Vector<ComponentBase**> rootData;
    get_root_component_data(rootData);

    for (ComponentBase** data : rootData)
        mObj->print_subtree(data, str, 0);

    return str;
}

} // namespace LD