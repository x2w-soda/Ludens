#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/Header/Hash.h>
#include <Ludens/Header/Math/Quat.h>
#include <Ludens/Header/Math/Transform.h>
#include <Ludens/Header/Math/Vec3.h>
#include <Ludens/Header/Types.h>

// NOTE: The DataComponent module is a high level module that drags
//       a lot of headers from different subsystems into the scope.

#include <Ludens/Asset/Asset.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/RenderServer/RServer.h>
#include <cstdint>

namespace LD {

/// @brief Unique identifier distributed by the DataRegistry. Zero is invalid ID.
using DUID = uint32_t;

/// @brief Use this concept to check if a type qualifies as a data component.
template <typename T>
concept IsDataComponent = LD::IsTrivial<T>;

enum ComponentType
{
    COMPONENT_TYPE_DATA = 0,
    COMPONENT_TYPE_TRANSFORM,
    COMPONENT_TYPE_MESH,
    COMPONENT_TYPE_TEXTURE_2D,
    COMPONENT_TYPE_ENUM_COUNT,
};

/// @brief Get the byte size of a data component.
size_t get_component_byte_size(ComponentType type);

/// @brief Get a static C string of the component type name.
const char* get_component_type_name(ComponentType type);

/// @brief Data component base members, hierarchy representation.
struct ComponentBase
{
    char* name;            /// user defined name
    ComponentBase* next;   /// next sibling component
    ComponentBase* child;  /// first child component
    ComponentBase* parent; /// parent component
    ComponentType type;    /// data component type
    DUID id;               /// data component ID
};

/// @brief Script attached to data component.
struct ComponentScriptSlot
{
    AUID assetID;     /// the script asset to instantiate from
    DUID componentID; /// the component this script slot belongs to
    bool isEnabled;   /// whether the script should be updated
};

/// @brief A component with only transform data.
struct TransformComponent
{
    Transform transform;
};

/// @brief Render data for meshes that do not deform.
/// @warning Placeholder implementation, this is very immature
struct MeshComponent
{
    Transform transform; /// mesh transform
    RUID ruid;           /// mesh render server handle
    AUID auid;           /// mesh asset handle
};

/// @brief Render data to draw a texture in 2D space
/// @warning Placeholder implementation, this is very immature
struct Texture2DComponent
{
    RImage image;          /// image handle
    Transform2D transform; /// texture 2D transform
    float zDepth;          /// texture z depth
    AUID auid;             /// texture asset handle
};

} // namespace LD