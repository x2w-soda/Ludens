#include <Ludens/Camera/CameraController.h>
#include <Ludens/Gizmo/Gizmo.h>
#include <Ludens/Header/Impulse.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/UI/UIImmediate.h>
#include <Ludens/WindowRegistry/Input.h>
#include <LudensEditor/EditorContext/EditorIconAtlas.h>
#include <LudensEditor/EditorContext/EditorWindow.h>
#include <LudensEditor/ViewportWindow/ViewportWindow.h>

#define GIZMO_SCREEN_SIZE_Y 150.0f
#define VIEWPORT_TOOLBAR_HEIGHT 26.0f

namespace LD {

/// @brief Editor viewport window implementation.
///        This window is a view into the Scene being edited.
///        Uses the Gizmo module to edit the object transforms.
struct ViewportWindowObj : EditorWindowObj
{
    EditorContext ctx;
    UIWorkspace space;
    UIWindow root;
    TransformEx subjectWorldTransform;
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
    Impulse isRequestingPlay;
    Impulse isRequestingStop;

    /// @brief Begin gizmo controls in the viewport
    void pick_gizmo(SceneOverlayGizmoID id);

    /// @brief Pick an object in the viewport
    void pick_ruid(RUID id);

    bool get_component_world_pos(CUID compID, Vec3& worldPos, Mat4& worldMat4);

    virtual EditorWindowType get_type() override { return EDITOR_WINDOW_VIEWPORT; }
    virtual void on_imgui(float delta) override;
    void update(float delta);
    void toolbar();
    void on_drag(MouseButton btn, const Vec2& dragPos, bool begin);

    static void on_editor_event(const EditorEvent* event, void* user);
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
    default:
        break;
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
    default:
        break;
    }

    return false;
}

static inline float get_plane_rotation(GizmoPlane plane, const Vec3& axisRotations)
{
    switch (plane)
    {
    case GIZMO_PLANE_XY:
        return (float)LD_TO_RADIANS(axisRotations.z);
    case GIZMO_PLANE_XZ:
        return (float)LD_TO_RADIANS(axisRotations.y);
    case GIZMO_PLANE_YZ:
        return (float)LD_TO_RADIANS(axisRotations.x);
    }

    LD_UNREACHABLE;
    return 0.0f;
}

void ViewportWindowObj::pick_gizmo(SceneOverlayGizmoID id)
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
            gizmo.begin_plane_rotate(plane, gizmoCenter, get_plane_rotation(plane, subjectWorldTransform.rotationEuler));
        break;
    case SCENE_OVERLAY_GIZMO_SCALE:
        if (get_gizmo_axis(id, axis))
            gizmo.begin_axis_scale(axis, gizmoCenter, subjectWorldTransform.scale);
        break;
    default:
        break;
    }
}

void ViewportWindowObj::pick_ruid(RUID id)
{
    subjectComp = ctx.get_ruid_component(id);
    ctx.set_selected_component(subjectComp);
}

bool ViewportWindowObj::get_component_world_pos(CUID compID, Vec3& worldPos, Mat4& worldMat4)
{
    if (!compID)
        return false;

    if (!ctx.get_component_transform_mat4(compID, worldMat4))
        return false;

    Vec3 localPos(0.0f);
    Vec4 worldPosW = worldMat4 * Vec4(localPos, 1.0f);
    worldPos = worldPosW.as_vec3() / worldPosW.w;

    return true;
}

void ViewportWindowObj::on_imgui(float delta)
{
    update(delta);

    EditorTheme theme = ctx.get_theme();
    UITheme uiTheme = theme.get_ui_theme();
    KeyCode key;
    MouseButton btn;
    Vec2 pos;
    bool begin;

    ui_push_window(root);

    if (ui_top_mouse_down(btn))
    {
        Vec2 pos;

        if (btn == MOUSE_BUTTON_RIGHT)
            enableCameraControls = true;
        else if (btn == MOUSE_BUTTON_LEFT && root.get_mouse_pos(pos))
        {
            pos = Vec2(pos.x, pos.y + VIEWPORT_TOOLBAR_HEIGHT);

            // update camera ray required for gizmo controls
            gizmo.update(editorCamera, pos, sceneExtent);

            if (hoverGizmoID != 0)
                pick_gizmo(hoverGizmoID);
            else if (hoverRUID != 0)
                pick_ruid(hoverRUID);
            else
                pick_ruid((RUID)0); // clear selection
        }
    }

    if (ui_top_mouse_up(btn))
    {
        if (btn == MOUSE_BUTTON_LEFT)
            gizmo.end();
        else if (btn == MOUSE_BUTTON_RIGHT)
            enableCameraControls = false;
    }

    if (ui_top_drag(btn, pos, begin))
        on_drag(btn, pos, begin);

    if (ui_top_key_down(key))
    {
        switch (key)
        {
        case KEY_CODE_1:
            gizmoType = SCENE_OVERLAY_GIZMO_TRANSLATION;
            break;
        case KEY_CODE_2:
            gizmoType = SCENE_OVERLAY_GIZMO_ROTATION;
            break;
        case KEY_CODE_3:
            gizmoType = SCENE_OVERLAY_GIZMO_SCALE;
            break;
        default:
            break;
        }
    }

    // toolbar widgets
    toolbar();

    ui_top_user(this);
    ui_top_draw([](UIWidget widget, ScreenRenderComponent renderer, void* user) {
        // draw scene image below toolbar
        Rect sceneRect = widget.get_rect();
        sceneRect.y += VIEWPORT_TOOLBAR_HEIGHT;
        sceneRect.h -= VIEWPORT_TOOLBAR_HEIGHT;
        RImage sceneImage = renderer.get_sampled_image();
        renderer.draw_image(sceneRect, sceneImage, 0xFFFFFFFF);
    });

    ui_pop_window();
}

void ViewportWindowObj::update(float delta)
{
    viewportExtent = root.get_rect().get_size();
    sceneExtent = Vec2(viewportExtent.x, viewportExtent.y - VIEWPORT_TOOLBAR_HEIGHT);
    editorCamera.set_aspect_ratio(sceneExtent.x / sceneExtent.y);

    // active mouse picking if cursor is within viewport window
    sceneMousePos = Vec2(-1.0f);

    if (root.get_mouse_pos(sceneMousePos))
    {
        // adjust for toolbar height
        sceneMousePos.y -= VIEWPORT_TOOLBAR_HEIGHT;
    }

    if (isRequestingPlay.read())
        ctx.play_scene();
    else if (isRequestingStop.read())
        ctx.stop_scene();

    // update gizmo scale from camera
    if (isGizmoVisible)
        gizmoScale = editorCamera.screen_to_world_size(gizmoCenter, sceneExtent.y, GIZMO_SCREEN_SIZE_Y);

    // update camera controls
    if (enableCameraControls)
    {
        CameraController cc = editorCameraController;

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
}

void ViewportWindowObj::toolbar()
{
    EditorTheme theme = ctx.get_theme();
    UITheme uiTheme = theme.get_ui_theme();
    MouseButton btn;

    UILayoutInfo layoutI{};
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fixed(VIEWPORT_TOOLBAR_HEIGHT);
    layoutI.childAxis = UI_AXIS_X;
    Color color = uiTheme.get_surface_color();

    // toolbar
    ui_push_panel(&color);
    ui_top_layout(layoutI);

    // translate gizmo
    float iconSize = VIEWPORT_TOOLBAR_HEIGHT;
    RImage iconAtlas = ctx.get_editor_icon_atlas();
    Rect iconRect = EditorIconAtlas::get_icon_rect(EditorIcon::Transform);
    ui_push_image(iconAtlas, iconSize, iconSize, 0xFFFFFFFF, &iconRect);
    ui_top_user(this);
    ui_top_draw([](UIWidget widget, ScreenRenderComponent renderer, void* user) {
        auto* obj = (ViewportWindowObj*)user;
        if (obj->gizmoType == SCENE_OVERLAY_GIZMO_TRANSLATION)
            renderer.draw_rect(widget.get_rect(), obj->ctx.get_theme().get_ui_theme().get_selection_color());
        UIImageWidget::on_draw(widget, renderer);
    });
    if (ui_top_mouse_down(btn) && btn == MOUSE_BUTTON_LEFT)
        gizmoType = SCENE_OVERLAY_GIZMO_TRANSLATION;
    ui_pop();

    // rotate gizmo
    iconRect = EditorIconAtlas::get_icon_rect(EditorIcon::Refresh);
    ui_push_image(iconAtlas, iconSize, iconSize, 0xFFFFFFFF, &iconRect);
    ui_top_user(this);
    ui_top_draw([](UIWidget widget, ScreenRenderComponent renderer, void* user) {
        auto* obj = (ViewportWindowObj*)user;
        if (obj->gizmoType == SCENE_OVERLAY_GIZMO_ROTATION)
            renderer.draw_rect(widget.get_rect(), obj->ctx.get_theme().get_ui_theme().get_selection_color());
        UIImageWidget::on_draw(widget, renderer);
    });
    if (ui_top_mouse_down(btn) && btn == MOUSE_BUTTON_LEFT)
        gizmoType = SCENE_OVERLAY_GIZMO_ROTATION;
    ui_pop();

    // scale gizmo
    iconRect = EditorIconAtlas::get_icon_rect(EditorIcon::LinearScale);
    ui_push_image(iconAtlas, iconSize, iconSize, 0xFFFFFFFF, &iconRect);
    ui_top_user(this);
    ui_top_draw([](UIWidget widget, ScreenRenderComponent renderer, void* user) {
        auto* obj = (ViewportWindowObj*)user;
        UITheme theme = obj->ctx.get_theme().get_ui_theme();
        if (obj->gizmoType == SCENE_OVERLAY_GIZMO_SCALE)
            renderer.draw_rect(widget.get_rect(), theme.get_selection_color());
        UIImageWidget::on_draw(widget, renderer);
    });
    if (ui_top_mouse_down(btn) && btn == MOUSE_BUTTON_LEFT)
        gizmoType = SCENE_OVERLAY_GIZMO_SCALE;
    ui_pop();

    // play / pause button
    bool isPlaying = ctx.is_playing();
    color = isPlaying ? theme.get_stop_button_color() : theme.get_play_button_color();
    iconRect = EditorIconAtlas::get_icon_rect(isPlaying ? EditorIcon::Close : EditorIcon::PlayArrow);
    ui_push_image(iconAtlas, iconSize, iconSize, color, &iconRect);
    if (ui_top_mouse_down(btn) && btn == MOUSE_BUTTON_LEFT)
    {
        if (isPlaying)
            ctx.stop_scene();
        else
            ctx.play_scene();
    }
    ui_pop();

    ui_pop();
}

void ViewportWindowObj::on_drag(MouseButton btn, const Vec2& dragPos, bool begin)
{
    if (btn != MOUSE_BUTTON_LEFT)
        return;

    // update gizmo controls
    GizmoAxis axis;
    GizmoPlane plane;
    GizmoControl control = gizmo.is_active(axis, plane);
    if (control == GIZMO_CONTROL_NONE)
        return;

    LD_ASSERT(subjectComp);
    TransformEx& worldT = subjectWorldTransform;
    const Vec2 scenePos(dragPos.x, dragPos.y - VIEWPORT_TOOLBAR_HEIGHT);

    // drag position is relative to window origin, i.e. already within sceneExtent range
    gizmo.update(editorCamera, scenePos, sceneExtent);

    switch (control)
    {
    case GIZMO_CONTROL_AXIS_TRANSLATION:
        worldT.position = gizmo.get_axis_translate();
        break;
    case GIZMO_CONTROL_PLANE_TRANSLATION:
        worldT.position = gizmo.get_plane_translate();
        break;
    case GIZMO_CONTROL_PLANE_ROTATION:
        switch (plane)
        {
        case GIZMO_PLANE_XY:
            worldT.rotationEuler.z = LD_TO_DEGREES(gizmo.get_plane_rotate());
            break;
        case GIZMO_PLANE_XZ:
            worldT.rotationEuler.y = LD_TO_DEGREES(gizmo.get_plane_rotate());
            break;
        case GIZMO_PLANE_YZ:
            worldT.rotationEuler.x = LD_TO_DEGREES(gizmo.get_plane_rotate());
            break;
        }
        worldT.rotation = Quat::from_euler(worldT.rotationEuler);
        break;
    case GIZMO_CONTROL_AXIS_SCALE:
        worldT.scale = gizmo.get_axis_scale();
        break;
    default:
        break;
    }

    // get inverse parent world matrix
    Mat4 parentInv(1.0f);
    const ComponentBase* base = ctx.get_component_base(subjectComp);
    if (base && base->parent)
    {
        Vec3 parentWorldPos;
        Mat4 parentWorldMat4(1.0f);
        get_component_world_pos(base->parent->id, parentWorldPos, parentWorldMat4);
        parentInv = Mat4::inverse(parentWorldMat4);
    }

    // get new local matrix from new world matrix and inverse parent world matrix
    Mat4 worldMat4 = worldT.as_mat4();
    Mat4 localMat4 = parentInv * worldMat4;

    // decompose local matrix to local transform
    TransformEx localTransform;
    bool ok = decompose_mat4_to_transform(localMat4, localTransform);
    LD_ASSERT(ok);
    ctx.set_component_transform(subjectComp, localTransform);

    // update gizmo center to new world space position
    get_component_world_pos(subjectComp, gizmoCenter, worldMat4);
}

void ViewportWindowObj::on_editor_event(const EditorEvent* event, void* user)
{
    auto& self = *(ViewportWindowObj*)user;

    if (event->type != EDITOR_EVENT_TYPE_NOTIFY_COMPONENT_SELECTION)
        return;

    const auto* selectionEvent = static_cast<const EditorNotifyComponentSelectionEvent*>(event);

    TransformEx localTransform;
    if (!selectionEvent->component || !self.ctx.get_selected_component_transform(localTransform))
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

//
// Public API
//

EditorWindow ViewportWindow::create(const EditorWindowInfo& windowI)
{
    ViewportWindowObj* obj = heap_new<ViewportWindowObj>(MEMORY_USAGE_UI);
    obj->ctx = windowI.ctx;
    obj->space = windowI.space;
    obj->root = obj->space.create_window(obj->space.get_root_id(), {}, {}, nullptr);
    obj->root.layout();
    obj->gizmo = Gizmo::create();
    obj->viewportExtent = obj->root.get_size();
    obj->sceneExtent = Vec2(obj->viewportExtent.x, obj->viewportExtent.y - VIEWPORT_TOOLBAR_HEIGHT);

    // TODO: parameterize camera && controller settings
    CameraPerspectiveInfo cameraPI{};
    cameraPI.aspectRatio = obj->sceneExtent.x / obj->sceneExtent.y;
    cameraPI.fov = 45.0f * LD_PI / 180.0f;
    cameraPI.nearClip = 0.1f;
    cameraPI.farClip = 100.0f;
    obj->editorCamera = Camera::create(cameraPI, {0.0f, 0.0f, 0.0f});
    obj->editorCamera.set_pos(Vec3(-2.10f, 0.05f, 11.64f));
    obj->editorCameraController = CameraController::create(obj->editorCamera, 3.0f, 0.22f);

    obj->gizmoType = SCENE_OVERLAY_GIZMO_TRANSLATION;
    obj->isGizmoVisible = false;

    obj->ctx.add_observer(&ViewportWindowObj::on_editor_event, obj);

    return {obj};
}

void ViewportWindow::destroy(EditorWindow viewport)
{
    LD_ASSERT(viewport && viewport.get_type() == EDITOR_WINDOW_VIEWPORT);

    auto* obj = static_cast<ViewportWindowObj*>(viewport.unwrap());

    CameraController::destroy(obj->editorCameraController);
    Camera::destroy(obj->editorCamera);
    Gizmo::destroy(obj->gizmo);

    heap_delete<ViewportWindowObj>(obj);
}

Camera ViewportWindow::get_editor_camera()
{
    return mObj->editorCamera;
}

Vec2 ViewportWindow::get_size()
{
    return mObj->viewportExtent;
}

Vec2 ViewportWindow::get_scene_size()
{
    return mObj->sceneExtent;
}

bool ViewportWindow::get_mouse_pos(Vec2& mousePos)
{
    if (mObj->sceneMousePos.x < 0.0f || mObj->sceneMousePos.y < 0.0f)
        return false;

    mousePos = mObj->sceneMousePos;
    return true;
}

void ViewportWindow::get_gizmo_state(SceneOverlayGizmo& gizmoType, Vec3& gizmoCenter, float& gizmoScale, RenderServerSceneGizmoColor& gizmoColor)
{
    if (!mObj->isGizmoVisible)
    {
        gizmoType = SCENE_OVERLAY_GIZMO_NONE;
        return;
    }

    gizmoType = mObj->gizmoType;
    gizmoCenter = mObj->gizmoCenter;
    gizmoScale = mObj->gizmoScale;

    EditorTheme theme = mObj->ctx.get_settings().get_theme();
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

void ViewportWindow::hover_id(SceneOverlayGizmoID gizmoID, RUID ruid)
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
