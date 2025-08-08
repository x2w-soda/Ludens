#pragma once

#include <Ludens/DataRegistry/DataComponent.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/Header/Hash.h>
#include <Ludens/System/Allocator.h>
#include <cstdint>

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
    /// @return Data component ID
    DUID create_component(ComponentType type, const char* name);

    /// @brief Destroy a data component subtree
    /// @param id Data component ID
    void destroy_component(DUID id);

    /// @brief Get data component base members, applicable to all types of components.
    const DataComponent* get_component_base(DUID id);

    /// @brief Get data component struct, or null on failure
    /// @param id Data component ID
    /// @param type Outputs data component type on success
    /// @return Address of some data component
    void* get_component(DUID id, ComponentType& type);

    /// @brief Get an iterator to traverse all components of a specific type.
    PoolAllocator::Iterator get_components(ComponentType type);

    /// @brief Get the Transform of a data component, or null if not applicable
    /// @return The address of the transform of a data component, or null
    /// @note Due to pointer stability of data components,
    ///       the returned address is also stable until the
    ///       destruction of the component.
    Transform* get_component_transform(DUID comp);

    /// @brief Get the render server ID of a data component, or null if not applicable
    /// @return The RUID associated with a data component, or null
    RUID get_component_ruid(DUID comp);
};

} // namespace LD