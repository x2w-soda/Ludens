#pragma once

#include <Ludens/Camera/Camera.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/Media/Bitmap.h>
#include <Ludens/Media/Font.h>
#include <Ludens/Media/Model.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/RenderComponent/SceneOverlayComponent.h>
#include <Ludens/RenderComponent/ScreenRenderComponent.h>
#include <Ludens/RenderServer/ScreenLayer.h>

namespace LD {

/// @brief Unique identifier distributed by the render server, zero is invalid ID
typedef uint32_t RUID;
typedef RUID CubemapDataID;
typedef RUID MeshDataID;
typedef RUID MeshDrawID;
typedef RUID Sprite2DDataID;
typedef RUID Sprite2DDrawID;

typedef void (*ScreenRenderCallback)(ScreenRenderComponent renderer, void* user);
typedef void (*RenderServerEditorRenderCallback)(ScreenRenderComponent renderer, void* user);
typedef void (*RenderServerEditorScenePickCallback)(SceneOverlayGizmoID gizmoID, RUID ruid, void* user);
typedef Mat4 (*RenderServerTransformCallback)(RUID ruid, void* user);
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
    RenderServerTransformCallback transformCallback; /// callback for server to grab the transform of objects
    void* user;                                      /// user of the scene render pass
    bool hasSkybox;                                  /// whether to draw skybox with the environment cubemap

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
    RenderServerScreenPassCallback callback;
    void* user; /// user of the scene screen pass
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
struct RenderServer : Handle<struct RenderServerObj>
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

    void editor_dialog_pass(const RenderServerEditorDialogPass& dialogPass);

    /// @brief Create standalone image from bitmap.
    RImage create_image(Bitmap bitmap);

    /// @brief Destroy standalone image.
    void destroy_image(RImage image);

    /// @brief Get the image handle of the font atlas image (RIMAGE_LAYOUT_SHADER_READ_ONLY).
    RImage get_font_atlas_image();

    struct ISprite2D : Handle<struct RenderServerObj>
    {
        bool exists(Sprite2DDataID dataID);
        Sprite2DDataID create_data_id(Bitmap bitmap);
        Sprite2DDrawID create_draw_id(Sprite2DDataID dataID);
        void destroy_draw_id(Sprite2DDrawID drawID);
        void destroy_all_draw_id();
    };

    /// @brief Access render server Sprite2D interface.
    inline ISprite2D sprite_2d() { return RenderServer::ISprite2D(mObj); }

    struct IMesh : Handle<struct RenderServerObj>
    {
        bool exists(MeshDataID dataID);
        MeshDataID create_data_id(ModelBinary& binary);
        MeshDrawID create_draw_id(MeshDataID dataID);
        void destroy_draw_id(MeshDrawID drawID);
        void destroy_all_data_id();
        void destroy_all_draw_id();
    };

    /// @brief Access render server mesh interface.
    inline IMesh mesh() { return RenderServer::IMesh(mObj); }

    struct ICubemap : Handle<struct RenderServerObj>
    {
        CubemapDataID create_data_id(Bitmap cubemapFaces);
        void destroy_data_id(CubemapDataID dataID);
    };

    /// @brief Access render server cubemap interface.
    inline ICubemap cubemap() { return RenderServer::ICubemap(mObj); }
};

} // namespace LD