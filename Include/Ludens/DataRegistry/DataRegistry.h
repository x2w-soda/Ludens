#pragma once

#include <Ludens/Asset/AssetRegistry.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/Header/Hash.h>
#include <Ludens/Header/Math/Transform.h>
#include <Ludens/Memory/Allocator.h>

#include <cstdint>

namespace LD {

struct ComponentBase;

/// @brief Component unique identifier distributed by the DataRegistry. Zero is invalid ID.
using CUID = uint64_t;

enum ComponentType
{
    COMPONENT_TYPE_DATA = 0,
    COMPONENT_TYPE_AUDIO_SOURCE,
    COMPONENT_TYPE_TRANSFORM,
    COMPONENT_TYPE_CAMERA,
    COMPONENT_TYPE_MESH,
    COMPONENT_TYPE_SPRITE_2D,
    COMPONENT_TYPE_ENUM_COUNT,
};

/// @brief The DataRegistry is the allocator of all DataComponents.
struct DataRegistry : Handle<struct DataRegistryObj>
{
    /// @brief Create empty registry
    static DataRegistry create();

    /// @brief Destroy registry and all components within
    static void destroy(DataRegistry registry);

    /// @brief Make a deep copy of the registry, mirroring the component hierarchy.
    ///        The duplicated components are not loaded, they have distinct CUIDs from
    ///        the original, but have the same SUIDs from the original.
    ///        The duplicate registry must also be destroyed by caller.
    DataRegistry duplicate() const;

    /// @brief Creates a data component
    /// @param type Component type
    /// @param name User defined name.
    /// @param parent Parent component, or 0 if creating a root component
    /// @param hint If not zero, create with this serial ID. Will fail if the serial ID is alraedy used.
    /// @return Data component ID
    CUID create_component(ComponentType type, const char* name, CUID parent, SUID hintSUID);

    /// @brief Destroy a data component subtree
    /// @param id Data component ID
    void destroy_component(CUID compID);

    /// @brief Reparent a data component
    void reparent(CUID compID, CUID parentID);

    /// @brief Get data component base members, applicable to all types of components.
    ComponentBase* get_component_base(CUID compID);

    /// @brief Get a representative asset of a component.
    AssetID get_component_asset_id(CUID compID);

    /// @brief Get component data, or null on failure.
    /// @param compID Data component ID
    /// @param outType If not null, outputs data component type on success
    /// @return Address of some data component
    ComponentBase** get_component_data(CUID compID, ComponentType* outType);

    /// @brief Get component data by serial ID, or null on failure.
    ComponentBase** get_component_data_by_suid(SUID compSUID, ComponentType* outType);

    /// @brief Get all data components with no parents.
    void get_root_component_data(Vector<ComponentBase**>& rootData);

    /// @brief Get an iterator to traverse all components of a specific type.
    PoolAllocator::Iterator get_components(ComponentType type);

    /// @brief Get the local transform of a data component.
    /// @return True on success.
    bool get_component_transform(CUID compID, TransformEx& transform);

    /// @brief Get the local transform of a data component.
    /// @return True on success.
    bool get_component_transform_2d(CUID compID, Transform2D& transform);

    /// @brief Set the local transform of a data component.
    /// @return True on success.
    bool set_component_transform(CUID compID, const TransformEx& transform);

    /// @brief Set the local transform of a data component.
    /// @return True on success.
    bool set_component_transform_2d(CUID compID, const Transform2D& transform);

    /// @brief Mark the transforms of a component subtree as dirty.
    /// @return True on success.
    bool mark_component_transform_dirty(CUID compID);

    /// @brief Get the Mat4 model matrix of a data component.
    /// @param compID Component ID to query.
    /// @param mat4 Output world space model matrix.
    /// @return True on success.
    bool get_component_world_mat4(CUID compID, Mat4& mat4);
};

/// @brief Get the byte size of a data component.
size_t get_component_byte_size(ComponentType type);

/// @brief Get a static C string of the component type name.
const char* get_component_type_name(ComponentType type);

} // namespace LD