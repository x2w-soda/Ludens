#pragma once

#include <Ludens/DataRegistry/DataRegistry.h>
#include <Ludens/Header/Bitwise.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/Header/Hash.h>
#include <Ludens/Header/Math/Quat.h>
#include <Ludens/Header/Math/Transform.h>
#include <Ludens/Header/Math/Vec3.h>
#include <Ludens/Header/Types.h>
#include <Ludens/Serial/SUID.h>

// NOTE: The DataComponent module is a high level module that drags
//       a lot of headers from different subsystems into the scope.

#include <Ludens/Asset/AssetRegistry.h>
#include <Ludens/AudioSystem/AudioSystem.h>
#include <Ludens/Camera/Camera.h>
#include <Ludens/Camera/Camera2D.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/RenderSystem/RenderSystem.h>
#include <Ludens/UI/UIWindow.h>
#include <cstdint>

namespace LD {

/// @brief Use this concept to check if a type qualifies as a data component.
template <typename T>
concept IsDataComponent = LD::IsTrivial<T>;

/// @brief Data component base members, hierarchy representation.
struct ComponentBase
{
    CUID cuid;             /// data component runtime ID
    char* name;            /// user defined name
    ComponentBase* next;   /// next sibling component
    ComponentBase* child;  /// first child component
    ComponentBase* parent; /// parent component
    union
    {
        TransformEx* transformEx;
        Transform2D* transform2D;
    };
    SUID suid;             /// data component serial ID
    ComponentType type;    /// data component type
    AssetID scriptAssetID; /// the script asset to instantiate from
};

/// @brief A component that emits sound.
struct AudioSourceComponent
{
    ComponentBase* base;
    AudioPlayback playback;
    AssetID clipID;
    float pan;
    float volumeLinear;
};

/// @brief A component with only transform data.
struct TransformComponent
{
    ComponentBase* base;
    TransformEx* transform;
};

/// @brief A component with only transform 2D data.
struct Transform2DComponent
{
    ComponentBase* base;
    Transform2D* transform;
};

/// @brief A camera in world space. The camera is responsible for
///        providing the view matrix and projection matrix during rendering.
struct CameraComponent
{
    ComponentBase* base;
    TransformEx* transform;
    Camera camera;
    bool isMainCamera;
};

struct Camera2DComponent
{
    ComponentBase* base;
    Transform2D* transform;
    Camera2D camera;
    Rect viewport; // normalized render region
    Camera2DConstraint constraint;
};

/// @brief Render data for meshes that do not deform.
/// @warning Placeholder implementation, this is very immature
struct MeshComponent
{
    ComponentBase* base;
    TransformEx* transform; /// mesh transform
    void* draw;             /// render server draw config
    AssetID assetID;        /// mesh asset id
};

/// @brief Render data to draw a texture in 2D space
/// @warning Placeholder implementation, this is very immature
struct Sprite2DComponent
{
    ComponentBase* base;
    Transform2D* transform; /// sprite 2D transform
    Sprite2DDraw draw;      /// render server draw config
    AssetID assetID;        /// texture asset handle
};

struct ScreenUIComponent
{
    ComponentBase* base;
    AssetID uiTemplateID; // UITemplateAsset
    class UIDriver* uiDriver;
    UIWindow uiWindow;
};

} // namespace LD