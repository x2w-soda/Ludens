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

    /// @brief Creates a data component
    /// @param type Root component type
    /// @param name User defined name.
    /// @param parent Parent component, or 0 if creating a root component
    /// @return Data component ID
    DUID create_component(ComponentType type, const char* name, DUID parent);

    /// @brief Destroy a data component subtree
    /// @param id Data component ID
    void destroy_component(DUID id);

    /// @brief Create data component script slot.
    ComponentScriptSlot* create_component_script_slot(DUID compID, AUID assetID);

    /// @brief Destroy data component script slot.
    void destroy_component_script_slot(DUID compID);

    /// @brief Get data component base members, applicable to all types of components.
    ComponentBase* get_component_base(DUID compID);

    /// @brief Get data component struct, or null on failure
    /// @param id Data component ID
    /// @param type Outputs data component type on success
    /// @return Address of some data component
    void* get_component(DUID id, ComponentType& type);

    /// @brief Get all data components with no parents.
    void get_root_components(std::vector<DUID>& roots);

    /// @brief Get an iterator to traverse all components of a specific type.
    PoolAllocator::Iterator get_components(ComponentType type);

    /// @brief Get script associated with a data component
    ComponentScriptSlot* get_component_script(DUID compID);

    /// @brief Get an iterator to traverse all scripts.
    PoolAllocator::Iterator get_component_scripts();

    /// @brief Get the Transform of a data component, or null if not applicable
    /// @return The address of the transform of a data component, or null
    /// @note Due to pointer stability of data components,
    ///       the returned address is also stable until the
    ///       destruction of the component.
    Transform* get_component_transform(DUID compID);

    /// @brief Get the render server ID of a data component, or null if not applicable
    /// @return The RUID associated with a data component, or null
    RUID get_component_ruid(DUID compID);
};

} // namespace LD