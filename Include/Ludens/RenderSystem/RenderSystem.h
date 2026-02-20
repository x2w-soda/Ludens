#pragma once

#include <Ludens/Camera/Camera.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/Header/IDHandle.h>
#include <Ludens/Header/Math/Viewport.h>
#include <Ludens/Media/Bitmap.h>
#include <Ludens/Media/Font.h>
#include <Ludens/Media/Model.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/RenderComponent/SceneOverlayComponent.h>
#include <Ludens/RenderComponent/ScreenRenderComponent.h>

#include <string>

namespace LD {

class RenderSystemObj;
class ScreenLayerObj;
struct Sprite2DDrawObj;
struct MeshDataObj;
struct MeshDrawObj;
struct RImageObj;

using Image2D = IDHandle<RImageObj, RUID>;
using ImageCube = IDHandle<RImageObj, RUID>;

typedef void (*ScreenRenderCallback)(ScreenRenderComponent renderer, void* user);
typedef void (*RenderSystemEditorScenePickCallback)(SceneOverlayGizmoID gizmoID, RUID ruid, void* user);
typedef bool (*RenderSystemMat4Callback)(RUID ruid, Mat4& mat4, void* user);

struct Sprite2DDraw : IDHandle<Sprite2DDrawObj, RUID>
{
    Sprite2DDraw() = default;
    Sprite2DDraw(Sprite2DDrawObj* obj, RUID id)
        : IDHandle(obj, id) {}

    bool set_image(Image2D image2D);
    Vec2 get_pivot();
    void set_pivot(const Vec2& pivot);
    uint32_t get_z_depth();
    void set_z_depth(uint32_t zDepth);
    Rect get_region();
    void set_region(const Rect& region);
    RUID get_layer_id();
};

struct MeshData : IDHandle<MeshDataObj, RUID>
{
    MeshData() = default;
    MeshData(MeshDataObj* obj, RUID id)
        : IDHandle(obj, id) {}
};

struct MeshDraw : IDHandle<MeshDrawObj, RUID>
{
    MeshDraw() = default;
    MeshDraw(MeshDrawObj* obj, RUID id)
        : IDHandle(obj, id) {}

    bool set_mesh_asset(MeshData data);
};

/// @brief Render system creation info
struct RenderSystemInfo
{
    RDevice device;      /// render device handle
    FontAtlas fontAtlas; /// default font atlas used for text rendering
};

/// @brief Info for the system to start a new frame
struct RenderSystemFrameInfo
{
    Vec2 screenExtent;     /// application screen extent
    Vec2 sceneExtent;      /// game scene extent
    Vec3 directionalLight; /// directional light vector
    RUID envCubemap;       /// optional environment cubemap to draw in scene
    WindowID dialogWindowID = 0;
    Vec4 clearColor;
};

struct RenderSystemSceneGizmoColor
{
    Color axisX;   /// color of X axis gizmo mesh
    Color axisY;   /// color of Y axis gizmo mesh
    Color axisZ;   /// color of Z axis gizmo mesh
    Color planeXY; /// color of XY plane gizmo mesh
    Color planeXZ; /// color of XZ plane gizmo mesh
    Color planeYZ; /// color of YZ plane gizmo mesh
};

/// @brief Render pass to draw the 3D world in Scene.
struct RenderSystemWorldPass
{
    RenderSystemMat4Callback mat4Callback; /// callback for system to grab the model matrix of 3D objects
    void* user;                            /// user of the scene render pass
    bool hasSkybox;                        /// whether to draw skybox with the environment cubemap
    Viewport worldViewport;

    // optional overlay rendering for gizmos and object outlining
    struct Overlay
    {
        bool enabled;                           /// probably true in Editor, false in Runtime
        RUID outlineRUID;                       /// mesh in scene to be outlined
        SceneOverlayGizmo gizmoType;            /// gizmo to render
        Vec3 gizmoCenter;                       /// gizmo center position
        float gizmoScale;                       /// gizmo size scale, default world size is 1x1x1
        RenderSystemSceneGizmoColor gizmoColor; /// gizmo mesh color for this frame
    } overlay;
};

/// @brief Render pass to draw 2D elements in Scene.
struct RenderSystemScreenPass
{
    RenderSystemMat4Callback mat4Callback; /// callback for system to grab the model matrix of 2D objects
    void* user;                            /// user of the scene screen pass

    struct Region
    {
        Viewport viewport;
        // TODO: ScreenLayer mask per-region
    };

    uint32_t regionCount;
    Region* regions;

    /// optional overlay to render on top of all regions, in practice this would be the screen UI
    struct Overlay
    {
        ScreenRenderCallback renderCallback;
        Viewport viewport;
    } overlay;
};

/// @brief Render pass to draw the Editor.
struct RenderSystemEditorPass
{
    const Vec2* sceneMousePickQuery;                       /// if not null, a mouse picking query within RSystemFrameInfo::sceneExtent
    ScreenRenderCallback renderCallback;                   /// for the Editor to render itself via a ScreenRenderComponent
    RenderSystemEditorScenePickCallback scenePickCallback; /// for the Editor to respond to scene mouse picking
    void* user;                                            /// user of the editor render pass
    Viewport viewport;                                     /// viewport to draw editor, likely full screen
};

/// @brief Render pass to draw an additional OS-level editor dialog Window.
struct RenderSystemEditorDialogPass
{
    ScreenRenderCallback renderCallback;
    WindowID dialogWindow;
    void* user;
};

/// @brief Render system handle. This is the top-level graphics abstraction,
///        Renderer resources are managed internally and are identified via a RUID.
struct RenderSystem : Handle<class RenderSystemObj>
{
    /// @brief Create the render system
    static RenderSystem create(const RenderSystemInfo& systemI);

    /// @brief Destroy the render system
    static void destroy(RenderSystem service);

    /// @brief Initiate the next GPU frame, this may block until the GPU has
    ///        finished processing the corresponding frame-in-flight. User must
    ///        also call submit() later.
    /// @param frameInfo parameters to use for this frame
    void next_frame(const RenderSystemFrameInfo& frameInfo);

    /// @brief Submit the frame for the GPU to process.
    void submit_frame();

    /// @brief Register world pass for this frame.
    void world_pass(const RenderSystemWorldPass& worldPass);

    /// @brief Register screen pass for this frame.
    void screen_pass(const RenderSystemScreenPass& screenPass);

    /// @brief Register editor pass for this frame. Not used in game Runtime.
    void editor_pass(const RenderSystemEditorPass& editorPass);

    /// @brief Register editor overlay pass for this frame. Not used in game Runtime.
    // void editor_overlay_pass(const RenderSystemEditorOverlayPass& editorPass);

    /// @brief Register editor dialog pass for this frame. Not used in game Runtime.
    void editor_dialog_pass(const RenderSystemEditorDialogPass& dialogPass);

    /// @brief Get the image handle of the font atlas image (RIMAGE_LAYOUT_SHADER_READ_ONLY).
    RImage get_font_atlas_image();

    Image2D create_image_2d(Bitmap bitmap);
    void destroy_image_2d(Image2D image);

    ImageCube create_image_cube(Bitmap cubemapFaces);
    void destroy_image_cube(ImageCube image);

    RUID create_screen_layer(const std::string& name);
    void destroy_screen_layer(RUID layer);

    Sprite2DDraw create_sprite_2d_draw(Image2D image2D, RUID layer);
    void destroy_sprite_2d_draw(Sprite2DDraw draw);

    MeshData create_mesh_data(ModelBinary& binary);
    void destroy_mesh_data(MeshData data);

    MeshDraw create_mesh_draw();
    MeshDraw create_mesh_draw(MeshData data);
    void destroy_mesh_draw(MeshDraw draw);
};

} // namespace LD