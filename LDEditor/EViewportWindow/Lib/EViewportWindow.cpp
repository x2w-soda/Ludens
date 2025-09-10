#include "ViewportToolbar.h"
#include <Ludens/Application/Input.h>
#include <Ludens/Camera/CameraController.h>
#include <Ludens/Gizmo/Gizmo.h>
#include <Ludens/System/Memory.h>
#include <LudensEditor/EViewportWindow/EViewportWindow.h>
#include <LudensEditor/EditorContext/EditorWindowObj.h>

#define GIZMO_SCREEN_SIZE_Y 150.0f

namespace LD {

/// @brief Editor viewport window implementation.
///        This window is a view into the Scene being edited.
///        Uses the Gizmo module to edit the object transforms.
struct EViewportWindowObj : EditorWindowObj
{
    virtual ~EViewportWindowObj() = default;

    ViewportToolbar toolbar;
    Transform subjectWorldTransform;
    CUID subjectComp;
    Camera editorCamera;
    CameraController editorCameraController;
    CameraPerspectiveInfo editorCameraPerspective;
    Gizmo gizmo;
    float gizmoScale;
    Vec3 gizmoCenter;                 /// gizmo center position in world space
    SceneOverlayGizmo gizmoType;      /// current gizmo control mode
    SceneOverlayGizmoID hoverGizmoID; /// the gizmo mesh under mouse cursor
    RUID hoverRUID;                   /// the mesh under mouse cursor
    Vec2 viewportExtent;              /// width and height of the entire viewport window
    Vec2 sceneExtent;                 /// width and height of the scene inside the viewport
    Vec2 sceneMousePos;               /// mouse position in sceneExtent
    bool isGizmoVisible;              /// whether gizmo meshes should be visible
    bool enableCameraControls;

    /// @brief Begin gizmo controls in the viewport
    void pick_gizmo(SceneOverlayGizmoID id);

    /// @brief Pick an object in the viewport
    void pick_ruid(RUID id);

    bool get_component_world_pos(CUID compID, Vec3& worldPos, Mat4& worldMat4);

    static void on_draw(UIWidget widget, ScreenRenderComponent renderer);
    static void on_key(UIWidget widget, KeyCode key, UIEvent event);
    static void on_mouse(UIWidget widget, const Vec2& pos, MouseButton btn, UIEvent event);
    static void on_drag(UIWidget widget, MouseButton btn, const Vec2& dragPos, bool begin);
    static void on_update(UIWidget widget, float delta);
    static void on_client_resize(UIWindow client, const Vec2& size);
    static void on_editor_context_event(const EditorContextEvent* event, void* user);
};

static inline bool get_gizmo_axis(SceneOverlayGizmoID id, GizmoAxis& axis)
{
    switch (id)
    {
    case SCENE_OVERLAY_GIZMO_ID_AXIS_X:
        axis = GIZMO_AXIS_X;
        return true;
    case SCENE_OVERLAY_GIZMO_ID_AXIS_Y:
        axis = GIZMO_AXIS_Y;
        return true;
    case SCENE_OVERLAY_GIZMO_ID_AXIS_Z:
        axis = GIZMO_AXIS_Z;
        return true;
    }

    return false;
}

static inline bool get_gizmo_plane(SceneOverlayGizmoID id, GizmoPlane& plane)
{
    switch (id)
    {
    case SCENE_OVERLAY_GIZMO_ID_PLANE_XY:
        plane = GIZMO_PLANE_XY;
        return true;
    case SCENE_OVERLAY_GIZMO_ID_PLANE_XZ:
        plane = GIZMO_PLANE_XZ;
        return true;
    case SCENE_OVERLAY_GIZMO_ID_PLANE_YZ:
        plane = GIZMO_PLANE_YZ;
        return true;
    }

    return false;
}

static inline float get_plane_rotation(GizmoPlane plane, const Vec3& axisRotations)
{
    switch (plane)
    {
    case GIZMO_PLANE_XY:
        return LD_TO_RADIANS(axisRotations.z);
    case GIZMO_PLANE_XZ:
        return LD_TO_RADIANS(axisRotations.y);
    case GIZMO_PLANE_YZ:
        return LD_TO_RADIANS(axisRotations.x);
    }

    LD_UNREACHABLE;
    return 0.0f;
}

void EViewportWindowObj::pick_gizmo(SceneOverlayGizmoID id)
{
    GizmoPlane plane;
    GizmoAxis axis;

    // writes back to subject transform during mouse drag window events
    // an object should be selected before gizmo mesh can even be selected
    LD_ASSERT(subjectComp);

    // initialize subject world transform
    Mat4 worldMat4;
    get_component_world_pos(subjectComp, gizmoCenter, worldMat4);
    bool ok = decompose_mat4_to_transform(worldMat4, subjectWorldTransform);
    LD_ASSERT(ok);

    switch (gizmoType)
    {
    case SCENE_OVERLAY_GIZMO_TRANSLATION:
        if (get_gizmo_axis(id, axis))
            gizmo.begin_axis_translate(axis, gizmoCenter);
        else if (get_gizmo_plane(id, plane))
            gizmo.begin_plane_translate(plane, gizmoCenter);
        break;
    case SCENE_OVERLAY_GIZMO_ROTATION:
        if (get_gizmo_plane(id, plane))
            gizmo.begin_plane_rotate(plane, gizmoCenter, get_plane_rotation(plane, subjectWorldTransform.rotation));
        break;
    case SCENE_OVERLAY_GIZMO_SCALE:
        if (get_gizmo_axis(id, axis))
            gizmo.begin_axis_scale(axis, gizmoCenter, subjectWorldTransform.scale);
        break;
    }
}

void EViewportWindowObj::pick_ruid(RUID id)
{
    subjectComp = editorCtx.get_ruid_component(id);
    editorCtx.set_selected_component(subjectComp);
}

bool EViewportWindowObj::get_component_world_pos(CUID compID, Vec3& worldPos, Mat4& worldMat4)
{
    if (!compID)
        return false;

    if (!editorCtx.get_component_transform_mat4(compID, worldMat4))
        return false;

    Vec3 localPos(0.0f);
    Vec4 worldPosW = worldMat4 * Vec4(localPos, 1.0f);
    worldPos = worldPosW.as_vec3() / worldPosW.w;

    return true;
}

void EViewportWindowObj::on_draw(UIWidget widget, ScreenRenderComponent renderer)
{
    EViewportWindowObj& self = *(EViewportWindowObj*)widget.get_user();

    // draw toolbar window, window manager won't draw it for us.
    self.toolbar.window.draw(renderer);

    // draw scene image
    float toolbarHeight = self.toolbar.window.get_size().y;
    Rect sceneRect = widget.get_rect();
    sceneRect.y += toolbarHeight;
    sceneRect.h -= toolbarHeight;
    RImage sceneImage = renderer.get_sampled_image();
    renderer.draw_image(sceneRect, sceneImage);
}

void EViewportWindowObj::on_key(UIWidget widget, KeyCode key, UIEvent event)
{
    EViewportWindowObj& self = *(EViewportWindowObj*)widget.get_user();

    if (event != UI_KEY_DOWN)
        return;

    switch (key)
    {
    case KEY_CODE_1:
        self.gizmoType = SCENE_OVERLAY_GIZMO_TRANSLATION;
        break;
    case KEY_CODE_2:
        self.gizmoType = SCENE_OVERLAY_GIZMO_ROTATION;
        break;
    case KEY_CODE_3:
        self.gizmoType = SCENE_OVERLAY_GIZMO_SCALE;
        break;
    }
}

void EViewportWindowObj::on_mouse(UIWidget widget, const Vec2& pos, MouseButton btn, UIEvent event)
{
    EViewportWindowObj& self = *(EViewportWindowObj*)widget.get_user();

    switch (event)
    {
    case UI_MOUSE_DOWN:
        if (btn == MOUSE_BUTTON_RIGHT)
        {
            self.enableCameraControls = true;
        }

        if (btn == MOUSE_BUTTON_LEFT)
        {
            // update camera ray required for gizmo controls
            self.gizmo.update(self.editorCamera, pos, self.sceneExtent);

            if (self.hoverGizmoID != 0)
                self.pick_gizmo(self.hoverGizmoID);
            else if (self.hoverRUID != 0)
                self.pick_ruid(self.hoverRUID);
            else
                self.pick_ruid((RUID)0); // clear selection
        }
        break;
    case UI_MOUSE_UP:
        if (btn == MOUSE_BUTTON_LEFT)
            self.gizmo.end();

        if (btn == MOUSE_BUTTON_RIGHT)
            self.enableCameraControls = false;
        break;
    }

}

void EViewportWindowObj::on_drag(UIWidget widget, MouseButton btn, const Vec2& dragPos, bool begin)
{
    EViewportWindowObj& self = *(EViewportWindowObj*)widget.get_user();

    if (btn != MOUSE_BUTTON_LEFT)
        return;

    // update gizmo controls
    GizmoAxis axis;
    GizmoPlane plane;
    GizmoControl control = self.gizmo.is_active(axis, plane);
    if (control == GIZMO_CONTROL_NONE)
        return;

    LD_ASSERT(self.subjectComp);
    Transform& worldT = self.subjectWorldTransform;

    // drag position is relative to window origin, i.e. already within sceneExtent range
    self.gizmo.update(self.editorCamera, dragPos, self.sceneExtent);

    switch (control)
    {
    case GIZMO_CONTROL_AXIS_TRANSLATION:
        worldT.position = self.gizmo.get_axis_translate();
        break;
    case GIZMO_CONTROL_PLANE_TRANSLATION:
        worldT.position = self.gizmo.get_plane_translate();
        break;
    case GIZMO_CONTROL_PLANE_ROTATION:
        switch (plane)
        {
        case GIZMO_PLANE_XY:
            worldT.rotation.z = LD_TO_DEGREES(self.gizmo.get_plane_rotate());
            break;
        case GIZMO_PLANE_XZ:
            worldT.rotation.y = LD_TO_DEGREES(self.gizmo.get_plane_rotate());
            break;
        case GIZMO_PLANE_YZ:
            worldT.rotation.x = LD_TO_DEGREES(self.gizmo.get_plane_rotate());
            break;
        }
        worldT.quat = Quat::from_euler(worldT.rotation);
        break;
    case GIZMO_CONTROL_AXIS_SCALE:
        worldT.scale = self.gizmo.get_axis_scale();
        break;
    }

    // get inverse parent world matrix
    Mat4 parentInv(1.0f);
    const ComponentBase* base = self.editorCtx.get_component_base(self.subjectComp);
    if (base && base->parent)
    {
        Vec3 parentWorldPos;
        Mat4 parentWorldMat4(1.0f);
        self.get_component_world_pos(base->parent->id, parentWorldPos, parentWorldMat4);
        parentInv = Mat4::inverse(parentWorldMat4);
    }

    // get new local matrix from new world matrix and inverse parent world matrix
    Mat4 worldMat4 = worldT.as_mat4();
    Mat4 localMat4 = parentInv * worldMat4;

    // decompose local matrix to local transform
    Transform localTransform;
    bool ok = decompose_mat4_to_transform(localMat4, localTransform);
    LD_ASSERT(ok);
    self.editorCtx.set_component_transform(self.subjectComp, localTransform);

    // update gizmo center to new world space position
    self.get_component_world_pos(self.subjectComp, self.gizmoCenter, worldMat4);
}

void EViewportWindowObj::on_update(UIWidget widget, float delta)
{
    auto& self = *(EViewportWindowObj*)widget.get_user();

    self.toolbar.window.set_pos(widget.get_pos());

    // active mouse picking if cursor is within viewport window
    self.sceneMousePos = Vec2(-1.0f);
    if (widget.get_mouse_pos(self.sceneMousePos))
    {
        // adjust for toolbar height
        self.sceneMousePos.y -= self.toolbar.window.get_size().y;
    }

    // TODO: move this to a play button?
    if (Input::get_key_down(KEY_CODE_SPACE))
        self.editorCtx.play_scene();
    if (Input::get_key_down(KEY_CODE_ESCAPE))
        self.editorCtx.stop_scene();

    // update gizmo scale from camera
    if (self.isGizmoVisible)
        self.gizmoScale = self.editorCamera.screen_to_world_size(self.gizmoCenter, self.sceneExtent.y, GIZMO_SCREEN_SIZE_Y);

    // update camera controls
    if (!self.enableCameraControls)
        return;

    CameraController cc = self.editorCameraController;

    if (Input::get_key(KEY_CODE_W))
        cc.move_forward();
    if (Input::get_key(KEY_CODE_S))
        cc.move_backward();

    if (Input::get_key(KEY_CODE_A))
        cc.move_left();
    if (Input::get_key(KEY_CODE_D))
        cc.move_right();

    if (Input::get_key(KEY_CODE_E))
        cc.move_world_up();
    if (Input::get_key(KEY_CODE_Q))
        cc.move_world_down();

    float dx, dy;
    if (Input::get_mouse_motion(dx, dy))
    {
        cc.view_pitch(-dy);
        cc.view_yaw(dx);
    }

    cc.update(delta);
}

void EViewportWindowObj::on_client_resize(UIWindow client, const Vec2& size)
{
    auto& self = *(EViewportWindowObj*)client.get_user();

    Rect toolbarRect = self.toolbar.window.get_rect();
    self.toolbar.window.set_size(Vec2(size.x, toolbarRect.h));

    self.viewportExtent = size;
    self.sceneExtent = Vec2(size.x, size.y - toolbarRect.h);
    self.editorCamera.set_aspect_ratio(size.x / size.y);
}

void EViewportWindowObj::on_editor_context_event(const EditorContextEvent* event, void* user)
{
    auto& self = *(EViewportWindowObj*)user;

    if (event->type != EDITOR_CONTEXT_EVENT_COMPONENT_SELECTION)
        return;

    const auto* selectionEvent = static_cast<const EditorContextComponentSelectionEvent*>(event);
    EditorContext ctx = self.editorCtx;

    Transform localTransform;
    if (!selectionEvent->component || !ctx.get_selected_component_transform(localTransform))
    {
        self.isGizmoVisible = false;
        self.subjectComp = 0;
        return;
    }

    Mat4 worldMat4;
    self.subjectComp = selectionEvent->component;
    self.isGizmoVisible = true;
    self.get_component_world_pos(self.subjectComp, self.gizmoCenter, worldMat4);
}

EViewportWindow EViewportWindow::create(const EViewportWindowInfo& windowI)
{
    UIWindowManager wm = windowI.wm;

    wm.set_window_title(windowI.areaID, "Viewport");
    wm.set_resize_callback(windowI.areaID, &EViewportWindowObj::on_client_resize);

    EViewportWindowObj* obj = heap_new<EViewportWindowObj>(MEMORY_USAGE_UI);
    obj->gizmo = Gizmo::create();
    obj->root = wm.get_area_window(windowI.areaID);
    obj->root.set_user(obj);
    obj->root.set_on_draw(&EViewportWindowObj::on_draw);
    obj->root.set_on_key(&EViewportWindowObj::on_key);
    obj->root.set_on_mouse(&EViewportWindowObj::on_mouse);
    obj->root.set_on_drag(&EViewportWindowObj::on_drag);
    obj->root.set_on_update(&EViewportWindowObj::on_update);
    obj->viewportExtent = obj->root.get_size();

    UIContext uiCtx = wm.get_context();
    float width = obj->viewportExtent.x;
    obj->toolbar.startup(uiCtx, width, &obj->gizmoType);
    uiCtx.layout();

    Rect toolbarRect = obj->toolbar.window.get_rect();
    obj->sceneExtent = Vec2(obj->viewportExtent.x, obj->viewportExtent.y - toolbarRect.h);
    float aspectRatio = obj->sceneExtent.x / obj->sceneExtent.y;

    // TODO: parameterize camera && controller settings
    CameraPerspectiveInfo cameraPI{};
    cameraPI.aspectRatio = aspectRatio;
    cameraPI.fov = 45.0f * LD_PI / 180.0f;
    cameraPI.nearClip = 0.1f;
    cameraPI.farClip = 100.0f;
    obj->editorCamera = Camera::create(cameraPI, {0.0f, 0.0f, 0.0f});
    obj->editorCamera.set_pos(Vec3(-2.10f, 0.05f, 11.64f));
    obj->editorCameraController = CameraController::create(obj->editorCamera, 3.0f, 0.22f);

    obj->gizmoType = SCENE_OVERLAY_GIZMO_TRANSLATION;
    obj->isGizmoVisible = false;

    obj->editorCtx = windowI.ctx;
    obj->editorCtx.add_observer(&EViewportWindowObj::on_editor_context_event, obj);

    return {obj};
}

void EViewportWindow::destroy(EViewportWindow viewport)
{
    EViewportWindowObj* obj = viewport;

    CameraController::destroy(obj->editorCameraController);
    Camera::destroy(obj->editorCamera);
    Gizmo::destroy(obj->gizmo);

    heap_delete<EViewportWindowObj>(obj);
}

Camera EViewportWindow::get_editor_camera()
{
    return mObj->editorCamera;
}

Vec2 EViewportWindow::get_size()
{
    return mObj->viewportExtent;
}

Vec2 EViewportWindow::get_scene_size()
{
    return mObj->sceneExtent;
}

bool EViewportWindow::get_mouse_pos(Vec2& mousePos)
{
    if (mObj->sceneMousePos.x < 0.0f || mObj->sceneMousePos.y < 0.0f)
        return false;

    mousePos = mObj->sceneMousePos;
    return true;
}

void EViewportWindow::get_gizmo_state(SceneOverlayGizmo& gizmoType, Vec3& gizmoCenter, float& gizmoScale, RServerSceneGizmoColor& gizmoColor)
{
    if (!mObj->isGizmoVisible)
    {
        gizmoType = SCENE_OVERLAY_GIZMO_NONE;
        return;
    }

    gizmoType = mObj->gizmoType;
    gizmoCenter = mObj->gizmoCenter;
    gizmoScale = mObj->gizmoScale;

    EditorTheme theme = mObj->editorCtx.get_settings().get_theme();
    Color gizmoAxisColors[3];
    theme.get_gizmo_colors(gizmoAxisColors[0], gizmoAxisColors[1], gizmoAxisColors[2]);

    Color gizmoPlaneColors[3] = {
        gizmoAxisColors[0],
        gizmoAxisColors[1],
        gizmoAxisColors[2],
    };

    Color highlightColor;
    theme.get_gizmo_highlight_color(highlightColor);

    GizmoAxis axis;
    GizmoPlane plane;
    GizmoControl control = mObj->gizmo.is_active(axis, plane);

    // highlight active gizmo
    switch (control)
    {
    case GIZMO_CONTROL_PLANE_ROTATION:
    case GIZMO_CONTROL_PLANE_TRANSLATION:
        gizmoPlaneColors[(int)plane - GIZMO_PLANE_XY] = highlightColor;
        break;
    case GIZMO_CONTROL_AXIS_SCALE:
    case GIZMO_CONTROL_AXIS_TRANSLATION:
        gizmoAxisColors[(int)axis - GIZMO_AXIS_X] = highlightColor;
        break;
    case GIZMO_CONTROL_NONE: // gizmo not active, highlight hovered gizmo
        switch (mObj->hoverGizmoID)
        {
        case SCENE_OVERLAY_GIZMO_ID_AXIS_X:
        case SCENE_OVERLAY_GIZMO_ID_AXIS_Y:
        case SCENE_OVERLAY_GIZMO_ID_AXIS_Z:
            gizmoAxisColors[(int)mObj->hoverGizmoID - SCENE_OVERLAY_GIZMO_ID_AXIS_X] = highlightColor;
            break;
        case SCENE_OVERLAY_GIZMO_ID_PLANE_XY:
        case SCENE_OVERLAY_GIZMO_ID_PLANE_XZ:
        case SCENE_OVERLAY_GIZMO_ID_PLANE_YZ:
            gizmoPlaneColors[(int)mObj->hoverGizmoID - SCENE_OVERLAY_GIZMO_ID_PLANE_XY] = highlightColor;
            break;
        }
    }

    gizmoColor.axisX = gizmoAxisColors[0];
    gizmoColor.axisY = gizmoAxisColors[1];
    gizmoColor.axisZ = gizmoAxisColors[2];
    gizmoColor.planeXY = gizmoPlaneColors[0];
    gizmoColor.planeXZ = gizmoPlaneColors[1];
    gizmoColor.planeYZ = gizmoPlaneColors[2];
}

void EViewportWindow::hover_id(SceneOverlayGizmoID gizmoID, RUID ruid)
{
    mObj->hoverGizmoID = (SceneOverlayGizmoID)0;
    mObj->hoverRUID = (RUID)0;

    if ((int)gizmoID != 0)
    {
        mObj->hoverGizmoID = gizmoID;
        mObj->hoverRUID = (RUID)0;
    }
    else if ((int)ruid != 0)
    {
        mObj->hoverGizmoID = (SceneOverlayGizmoID)0;
        mObj->hoverRUID = ruid;
    }
}

} // namespace LD