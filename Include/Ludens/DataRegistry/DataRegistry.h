#pragma once

#include <Ludens/Asset/AssetRegistry.h>
#include <Ludens/DSA/IDRegistry.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/DataRegistry/DataRegistryDef.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/Header/Hash.h>
#include <Ludens/Header/Math/Transform.h>
#include <Ludens/Memory/Allocator.h>

#include <cstdint>

namespace LD {

struct ComponentBase;

/// @brief Component unique identifier distributed by an IDRegistry. Zero is invalid ID.
using CUID = ID;

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
    /// @param suid Serial ID of the component.
    /// @return Data component ID
    CUID create_component(ComponentType type, const char* name, CUID parent, SUID suid);

    /// @brief Destroy a component subtree.
    /// @param id Data component ID
    void destroy_component_subtree(CUID compID);

    /// @brief Reparent a component subtree.
    void reparent_component_subtree(CUID compID, CUID parentID);

    /// @brief Try clone a component subtree.
    /// @param rootID The subtree root component to clone.
    /// @param suidRegistry Used to generate new serial IDs
    /// @return Data of the root component of the cloned subtree on success.
    ComponentBase** clone_component_subtree(CUID rootID, SUIDRegistry suidRegistry);

    /// @brief Get data component base members, applicable to all types of components.
    ComponentBase* get_component_base(CUID compID);

    /// @brief Get a representative asset of a component.
    AssetID get_component_asset_id(CUID compID);

    void set_component_name(CUID compID, const char* cstr);

    /// @brief Get component data, or null on failure.
    /// @param compID Data component ID
    /// @param outType If not null, outputs data component type on success
    /// @return Address of some data component
    ComponentBase** get_component_data(CUID compID, ComponentType* outType);

    /// @brief Get component data by serial ID, or null on failure.
    ComponentBase** get_component_data_by_suid(SUID compSUID, ComponentType* outType);

    /// @brief Get component data by sibling index path.
    /// @note Slower code path intended for Editor.
    ComponentBase** get_component_data_by_path(const Vector<int>& path);

    /// @brief Get all data components with no parents.
    void get_root_component_data(Vector<ComponentBase**>& rootData);

    /// @brief Get an iterator to traverse all components of a specific type.
    PoolAllocator::Iterator get_components(ComponentType type);

    /// @brief Get a path of sibling indices.
    /// @return True on success.
    /// @note Slower code path intended for editor, sibling indices are calculated on the fly.
    bool get_component_path(CUID compID, Vector<int>& path);

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

    /// @brief Get the Mat4 model matrix of a data component.
    /// @param compID Component ID to query.
    /// @param mat4 Output world space model matrix.
    /// @return True on success.
    bool get_component_world_mat4(CUID compID, Mat4& mat4);

    bool get_component_world_transform_2d(CUID compID, Transform2D& transform);

    void invalidate_transforms();

    std::string print_hierarchy();
};

/// @brief Get the byte size of a data component.
size_t get_component_byte_size(ComponentType type);

/// @brief Get a static C string of the component type name.
const char* get_component_type_name(ComponentType type);

} // namespace LD