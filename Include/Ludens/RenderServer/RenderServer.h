#pragma once

#include <Ludens/Camera/Camera.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/Media/Bitmap.h>
#include <Ludens/Media/Font.h>
#include <Ludens/Media/Model.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/RenderComponent/SceneOverlayComponent.h>
#include <Ludens/RenderComponent/ScreenRenderComponent.h>
#include <Ludens/RenderServer/RenderServerObj.h>

#include <string>

namespace LD {

typedef void (*ScreenRenderCallback)(ScreenRenderComponent renderer, void* user);
typedef void (*RenderServerEditorRenderCallback)(ScreenRenderComponent renderer, void* user);
typedef void (*RenderServerEditorScenePickCallback)(SceneOverlayGizmoID gizmoID, RUID ruid, void* user);
typedef Mat4 (*RenderServerMat4Callback)(RUID ruid, void* user);
typedef void (*RenderServerScreenPassCallback)(ScreenRenderComponent renderer, void* user);

/// @brief Render server creation info
struct RenderServerInfo
{
    RDevice device;      /// render device handle
    FontAtlas fontAtlas; /// default font atlas used for text rendering
};

/// @brief Info for the server to start a new frame
struct RenderServerFrameInfo
{
    Camera mainCamera;     /// main camera to view the scene from
    Vec2 screenExtent;     /// application screen extent
    Vec2 sceneExtent;      /// game scene extent
    Vec3 directionalLight; /// directional light vector
    RUID envCubemap;       /// optional environment cubemap to draw in scene
    WindowID dialogWindowID = 0;
    Vec4 clearColor;
};

struct RenderServerSceneGizmoColor
{
    Color axisX;   /// color of X axis gizmo mesh
    Color axisY;   /// color of Y axis gizmo mesh
    Color axisZ;   /// color of Z axis gizmo mesh
    Color planeXY; /// color of XY plane gizmo mesh
    Color planeXZ; /// color of XZ plane gizmo mesh
    Color planeYZ; /// color of YZ plane gizmo mesh
};

/// @brief Info for the server to render the game scene
struct RenderServerScenePass
{
    RenderServerMat4Callback mat4Callback; /// callback for server to grab the model matrix of 3D objects
    void* user;                            /// user of the scene render pass
    bool hasSkybox;                        /// whether to draw skybox with the environment cubemap

    // optional overlay rendering for gizmos and object outlining
    struct
    {
        bool enabled;                           /// probably true in Editor, false in Runtime
        RUID outlineRUID;                       /// mesh in scene to be outlined
        SceneOverlayGizmo gizmoType;            /// gizmo to render
        Vec3 gizmoCenter;                       /// gizmo center position
        float gizmoScale;                       /// gizmo size scale, default world size is 1x1x1
        RenderServerSceneGizmoColor gizmoColor; /// gizmo mesh color for this frame
    } overlay;
};

/// @brief Info for the server to render in screen space on top of scene.
struct RenderServerScreenPass
{
    RenderServerMat4Callback mat4Callback;   /// callback for server to grab the model matrix of 2D objects
    RenderServerScreenPassCallback callback; /// optional hook to render on top of all ScreenLayers
    void* user;                              /// user of the scene screen pass
};

/// @brief Info for the server to render the editor
struct RenderServerEditorPass
{
    const Vec2* sceneMousePickQuery;                       /// if not null, a mouse picking query within RServerFrameInfo::sceneExtent
    RenderServerEditorRenderCallback renderCallback;       /// for the Editor to render itself via a ScreenRenderComponent
    RenderServerEditorScenePickCallback scenePickCallback; /// for the Editor to respond to scene mouse picking
    void* user;                                            /// user of the editor render pass
};

/// @brief Info for the server to render the editor overlay
struct RenderServerEditorOverlayPass
{
    RenderServerEditorRenderCallback renderCallback; /// for the Editor to render additional overlays after the base pass
    Color blurMixColor;                              /// mix color RGB for the blurred editor background, keep alpha channel at 0xFF
    float blurMixFactor;                             /// lerp factor between blur color and mix color, 0 performs no blur
    void* user;                                      /// user of the editor overlay render pass
};

/// @brief Info for the server to render a dialog Window in screen space.
struct RenderServerEditorDialogPass
{
    ScreenRenderCallback renderCallback;
    WindowID dialogWindow;
    void* user;
};

/// @brief Render server handle. This is the top-level graphics abstraction,
///        Renderer resources are managed internally and are identified via a RUID.
struct RenderServer : Handle<class RenderServerObj>
{
    /// @brief Create the render server
    static RenderServer create(const RenderServerInfo& serverI);

    /// @brief Destroy the render server
    static void destroy(RenderServer service);

    /// @brief Initiate the next GPU frame, this may block until the GPU has
    ///        finished processing the corresponding frame-in-flight. User must
    ///        also call submit() later.
    /// @param frameInfo parameters to use for this frame
    void next_frame(const RenderServerFrameInfo& frameInfo);

    /// @brief Submit the frame for the GPU to process.
    void submit_frame();

    /// @brief Base pass to render the game scene.
    void scene_pass(const RenderServerScenePass& sceneRP);

    /// @brief Screen pass to render on top of game scene.
    void screen_pass(const RenderServerScreenPass& screenP);

    /// @brief Dependency injection for the Editor to render itself.
    ///        Not used in game Runtime.
    void editor_pass(const RenderServerEditorPass& editorPass);

    /// @brief Dependency injection for the Editor to render more stuff on top of the editor pass.
    ///        Not used in game Runtime.
    void editor_overlay_pass(const RenderServerEditorOverlayPass& editorPass);

    /// @brief Optional pass for the Editor to render a dialog window.
    void editor_dialog_pass(const RenderServerEditorDialogPass& dialogPass);

    /// @brief Get the image handle of the font atlas image (RIMAGE_LAYOUT_SHADER_READ_ONLY).
    RImage get_font_atlas_image();

    Image2D create_image_2d(Bitmap bitmap);
    void destroy_image_2d(Image2D image);

    ImageCube create_image_cube(Bitmap cubemapFaces);
    void destroy_image_cube(ImageCube image);

    RUID create_screen_layer(const std::string& name);
    void destroy_screen_layer(RUID layer);

    Sprite2DDraw create_sprite_2d_draw(Image2D image2D, RUID layer, const Rect& rect, uint32_t zDepth);
    void destroy_sprite_2d_draw(Sprite2DDraw draw);

    MeshData create_mesh_data(ModelBinary& binary);
    void destroy_mesh_data(MeshData data);

    MeshDraw create_mesh_draw();
    MeshDraw create_mesh_draw(MeshData data);
    void destroy_mesh_draw(MeshDraw draw);
};

} // namespace LD