#pragma once

#include <Ludens/DataRegistry/DataComponent.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/Header/Hash.h>
#include <Ludens/System/Allocator.h>
#include <cstdint>
#include <vector>

namespace LD {

/// @brief The DataRegistry is the allocator of all DataComponents.
struct DataRegistry : Handle<struct DataRegistryObj>
{
    /// @brief Create empty registry
    static DataRegistry create();

    /// @brief Destroy registry and all components within
    static void destroy(DataRegistry registry);

    /// @brief Make a deep copy of the registry. This duplicates the entire component hierarchy
    ///        using new allocators separate from the original. The copy must be destroyed later.
    DataRegistry duplicate() const;

    /// @brief Creates a data component
    /// @param type Component type
    /// @param name User defined name.
    /// @param parent Parent component, or 0 if creating a root component
    /// @param hint If not zero, create with this hint ID. Will fail if the ID is alraedy used.
    /// @return Data component ID
    CUID create_component(ComponentType type, const char* name, CUID parent, CUID hint);

    /// @brief Destroy a data component subtree
    /// @param id Data component ID
    void destroy_component(CUID id);

    /// @brief Reparent a data component
    void reparent(CUID compID, CUID parentID);

    /// @brief Create data component script slot.
    ComponentScriptSlot* create_component_script_slot(CUID compID, AUID assetID);

    /// @brief Destroy data component script slot.
    void destroy_component_script_slot(CUID compID);

    /// @brief Get data component base members, applicable to all types of components.
    ComponentBase* get_component_base(CUID compID);

    /// @brief Get a representative asset of a component.
    AUID get_component_auid(CUID compID);

    /// @brief Get data component struct, or null on failure
    /// @param compID Data component ID
    /// @param type Outputs data component type on success
    /// @return Address of some data component
    void* get_component(CUID compID, ComponentType& type);

    /// @brief Get all data components with no parents.
    void get_root_components(std::vector<CUID>& roots);

    /// @brief Get an iterator to traverse all components of a specific type.
    PoolAllocator::Iterator get_components(ComponentType type);

    /// @brief Get script associated with a data component
    ComponentScriptSlot* get_component_script(CUID compID);

    /// @brief Get an iterator to traverse all scripts.
    PoolAllocator::Iterator get_component_scripts();

    /// @brief Get the local transform of a data component.
    /// @return True on success.
    bool get_component_transform(CUID compID, Transform& transform);

    /// @brief Set the local transform of a data component.
    /// @return True on success.
    bool set_component_transform(CUID compID, const Transform& transform);

    /// @brief Get the local 2D transform of a data component.
    /// @return True on success.
    bool get_component_transform2d(CUID compID, Transform2D& transform);

    /// @brief Mark the transforms of a component subtree as dirty.
    /// @return True on success.
    bool mark_component_transform_dirty(CUID compID);

    /// @brief Get the model matrix of a data component.
    /// @param compID Component ID to query.
    /// @param mat4 Output world space model matrix.
    /// @return True on success.
    bool get_component_transform_mat4(CUID compID, Mat4& mat4);
};

} // namespace LD