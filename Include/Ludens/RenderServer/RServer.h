#pragma once

#include <Ludens/Camera/Camera.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/Media/Bitmap.h>
#include <Ludens/Media/Font.h>
#include <Ludens/Media/Model.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/RenderComponent/SceneOverlayComponent.h>
#include <Ludens/RenderComponent/ScreenRenderComponent.h>

namespace LD {

/// @brief Unique identifier distributed by the render server, zero is invalid ID
typedef uint32_t RUID;

typedef void (*RServerEditorRenderCallback)(ScreenRenderComponent renderer, void* user);
typedef void (*RServerEditorScenePickCallback)(SceneOverlayGizmoID gizmoID, RUID ruid, void* user);
typedef Mat4 (*RServerTransformCallback)(RUID ruid, void* user);

/// @brief Render server creation info
struct RServerInfo
{
    RDevice device;      /// render device handle
    FontAtlas fontAtlas; /// default font atlas used for text rendering
    Bitmap cubemapFaces; /// default 6 faces of default environment cubemap
};

/// @brief Info for the server to start a new frame
struct RServerFrameInfo
{
    Camera mainCamera;     /// main camera to view the scene from
    Vec2 screenExtent;     /// application screen extent
    Vec2 sceneExtent;      /// game scene extent
    Vec3 directionalLight; /// directional light vector
};

struct RServerSceneGizmoColor
{
    Color axisX;   /// color of X axis gizmo mesh
    Color axisY;   /// color of Y axis gizmo mesh
    Color axisZ;   /// color of Z axis gizmo mesh
    Color planeXY; /// color of XY plane gizmo mesh
    Color planeXZ; /// color of XZ plane gizmo mesh
    Color planeYZ; /// color of YZ plane gizmo mesh
};

/// @brief Info for the server to render the game scene
struct RServerScenePass
{
    RServerTransformCallback transformCallback; /// callback for server to grab the transform of objects
    void* user;                                 /// user of the scene render pass

    // optional overlay rendering for gizmos and object outlining
    struct
    {
        bool enabled;                      /// probably true in Editor, false in Runtime
        RUID outlineRUID;                  /// mesh in scene to be outlined
        SceneOverlayGizmo gizmoType;       /// gizmo to render
        Vec3 gizmoCenter;                  /// gizmo center position
        float gizmoScale;                  /// gizmo size scale, default world size is 1x1x1
        RServerSceneGizmoColor gizmoColor; /// gizmo mesh color for this frame
    } overlay;
};

/// @brief Info for the server to render the editor
struct RServerEditorPass
{
    const Vec2* sceneMousePickQuery;                  /// if not null, a mouse picking query within RServerFrameInfo::sceneExtent
    RServerEditorRenderCallback renderCallback;       /// for the Editor to render itself via a ScreenRenderComponent
    RServerEditorScenePickCallback scenePickCallback; /// for the Editor to respond to scene mouse picking
    void* user;                                       /// user of the editor render pass
};

/// @brief Info for the server to render the editor overlay
struct RServerEditorOverlayPass
{
    RServerEditorRenderCallback renderCallback; /// for the Editor to render additional overlays after the base pass
    void* user;                                 /// user of the editor overlay render pass
};

/// @brief Render server handle. This is the top-level graphics abstraction,
///        Renderer resources are managed internally and are identified via a RUID.
struct RServer : Handle<struct RServerObj>
{
    /// @brief Create the render server
    static RServer create(const RServerInfo& serverI);

    /// @brief Destroy the render server
    static void destroy(RServer service);

    /// @brief Initiate the next GPU frame, this may block until the GPU has
    ///        finished processing the corresponding frame-in-flight. User must
    ///        also call submit() later.
    /// @param frameInfo parameters to use for this frame
    void next_frame(const RServerFrameInfo& frameInfo);

    /// @brief Submit the frame for the GPU to process.
    void submit_frame();

    /// @brief Base pass to render the game scene.
    void scene_pass(const RServerScenePass& sceneRP);

    /// @brief Dependency injection for the Editor to render itself.
    ///        Not used in game Runtime.
    void editor_pass(const RServerEditorPass& editorPass);

    /// @brief Dependency injection for the Editor to render more stuff on top of the editor pass.
    ///        Not used in game Runtime.
    void editor_overlay_pass(const RServerEditorOverlayPass& editorPass);

    /// @brief Get the underlying render device.
    RDevice get_device();

    /// @brief Get the image handle of the font atlas image (RIMAGE_LAYOUT_SHADER_READ_ONLY).
    RImage get_font_atlas_image();

    /// @brief Create a non-deforming mesh
    RUID create_mesh(ModelBinary& modelBinary);
};

} // namespace LD