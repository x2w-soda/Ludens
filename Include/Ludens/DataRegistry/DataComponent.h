#pragma once

#include <Ludens/Header/Bitwise.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/Header/Hash.h>
#include <Ludens/Header/Math/Quat.h>
#include <Ludens/Header/Math/Transform.h>
#include <Ludens/Header/Math/Vec3.h>
#include <Ludens/Header/Types.h>

// NOTE: The DataComponent module is a high level module that drags
//       a lot of headers from different subsystems into the scope.

#include <Ludens/Asset/Asset.h>
#include <Ludens/AudioSystem/AudioSystem.h>
#include <Ludens/Camera/Camera.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/RenderSystem/RenderSystem.h>
#include <cstdint>

namespace LD {

/// @brief Component unique identifier distributed by the DataRegistry. Zero is invalid ID.
using CUID = uint32_t;

/// @brief Use this concept to check if a type qualifies as a data component.
template <typename T>
concept IsDataComponent = LD::IsTrivial<T>;

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

using ComponentFlag = uint32_t;

enum ComponentFlagBit : uint32_t
{
    COMPONENT_FLAG_TRANSFORM_DIRTY_BIT = LD_BIT(2),
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
    CUID id;               /// data component ID
    ComponentFlag flags;   /// data component flags
    Mat4 localMat4;        /// transform matrix relative to parent
    Mat4 worldMat4;        /// world space model matrix
};

/// @brief Script attached to data component.
struct ComponentScriptSlot
{
    AUID assetID;     /// the script asset to instantiate from
    CUID componentID; /// the component this script slot belongs to
    bool isEnabled;   /// whether the script should be updated
};

/// @brief A component that emits sound.
struct AudioSourceComponent
{
    ComponentBase* base;
    AudioPlayback playback;
    AUID clipAUID;
    float pan;
    float volumeLinear;
};

/// @brief A component with only transform data.
struct TransformComponent
{
    ComponentBase* base;
    TransformEx transform;
};

/// @brief A camera in world space. The camera is responsible for
///        providing the view matrix and projection matrix during rendering.
struct CameraComponent
{
    ComponentBase* base;
    TransformEx transform;
    Camera camera;
    union
    {
        CameraPerspectiveInfo perspective;
        CameraOrthographicInfo orthographic;
    };
    bool isMainCamera;
    bool isPerspective;
};

/// @brief Render data for meshes that do not deform.
/// @warning Placeholder implementation, this is very immature
struct MeshComponent
{
    ComponentBase* base;
    TransformEx transform; /// mesh transform
    MeshDraw draw;         /// render server draw config
    AUID auid;             /// mesh asset handle
};

/// @brief Render data to draw a texture in 2D space
/// @warning Placeholder implementation, this is very immature
struct Sprite2DComponent
{
    ComponentBase* base;
    Transform2D transform; /// sprite 2D transform
    Sprite2DDraw draw;     /// render server draw config
    AUID auid;             /// texture asset handle
};

} // namespace LD