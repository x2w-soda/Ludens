#include <Ludens/Header/Impulse.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/UI/UIImmediate.h>
#include <LudensEditor/EditorContext/EditorIconAtlas.h>
#include <LudensEditor/EditorContext/EditorWindow.h>
#include <LudensEditor/ViewportWindow/ViewportWindow.h>

#include "Viewport2D.h"
#include "Viewport3D.h"
#include "ViewportCommon.h"

namespace LD {

enum ViewportMode
{
    VIEWPORT_MODE_2D,
    VIEWPORT_MODE_3D
};

/// @brief Editor viewport window implementation.
///        This window is a view into the Scene being edited.
///        Uses the Gizmo module to edit the object transforms.
struct ViewportWindowObj : EditorWindowObj
{
    EditorContext ctx;
    UIWorkspace space;
    UIWindow root;
    ViewportMode mode = VIEWPORT_MODE_2D;
    Viewport2D viewport2D{};
    Viewport3D viewport3D{};
    ViewportState state{}; /// passed down to Viewport2D or Viewport3D each frame
    Impulse isRequestingPlay{};
    Impulse isRequestingStop{};

    ViewportWindowObj() = default;
    ViewportWindowObj(const ViewportWindowObj&) = delete;
    ~ViewportWindowObj() = default;

    ViewportWindowObj& operator=(const ViewportWindowObj&) = delete;

    virtual EditorWindowType get_type() override { return EDITOR_WINDOW_VIEWPORT; }
    virtual void on_imgui(float delta) override;
    void toolbar();
    void viewport_editor_imgui(ViewportState& state);
    void viewport_scene_imgui();

    static void on_editor_event(const EditorEvent* event, void* user);
};

void ViewportWindowObj::on_imgui(float delta)
{
    EditorTheme theme = ctx.get_theme();
    UITheme uiTheme = theme.get_ui_theme();

    // update state for this frame
    state.delta = delta;
    state.viewportExtent = root.get_rect().get_size();
    state.sceneExtent = Vec2(state.viewportExtent.x, state.viewportExtent.y - VIEWPORT_TOOLBAR_HEIGHT);

    // active mouse picking if cursor is within viewport window
    state.sceneMousePos = Vec2(-1.0f);

    if (root.get_mouse_pos(state.sceneMousePos))
    {
        // adjust for toolbar height
        state.sceneMousePos.y -= VIEWPORT_TOOLBAR_HEIGHT;
    }

    // TODO: this should come after toolbar?
    if (isRequestingPlay.read())
        ctx.play_scene();
    else if (isRequestingStop.read())
        ctx.stop_scene();

    ui_push_window(root);

    // Input routing will depend on whether the scene is playing
    // and whether we are in 2D or 3D editor mode.
    if (ctx.is_playing())
        viewport_scene_imgui();
    else
        viewport_editor_imgui(state);

    // toolbar widgets
    toolbar();

    // draw scene image below toolbar
    ui_top_user(this);
    ui_top_draw([](UIWidget widget, ScreenRenderComponent renderer, void* user) {
        Rect sceneRect = widget.get_rect();
        sceneRect.y += VIEWPORT_TOOLBAR_HEIGHT;
        sceneRect.h -= VIEWPORT_TOOLBAR_HEIGHT;
        RImage sceneImage = renderer.get_sampled_image();
        renderer.draw_image(sceneRect, sceneImage, 0xFFFFFFFF);
    });

    ui_pop_window();
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
        if (obj->state.gizmoType == SCENE_OVERLAY_GIZMO_TRANSLATION)
            renderer.draw_rect(widget.get_rect(), obj->ctx.get_theme().get_ui_theme().get_selection_color());
        UIImageWidget::on_draw(widget, renderer);
    });
    if (ui_top_mouse_down(btn) && btn == MOUSE_BUTTON_LEFT)
        state.gizmoType = SCENE_OVERLAY_GIZMO_TRANSLATION;
    ui_pop();

    // rotate gizmo
    iconRect = EditorIconAtlas::get_icon_rect(EditorIcon::Refresh);
    ui_push_image(iconAtlas, iconSize, iconSize, 0xFFFFFFFF, &iconRect);
    ui_top_user(this);
    ui_top_draw([](UIWidget widget, ScreenRenderComponent renderer, void* user) {
        auto* obj = (ViewportWindowObj*)user;
        if (obj->state.gizmoType == SCENE_OVERLAY_GIZMO_ROTATION)
            renderer.draw_rect(widget.get_rect(), obj->ctx.get_theme().get_ui_theme().get_selection_color());
        UIImageWidget::on_draw(widget, renderer);
    });
    if (ui_top_mouse_down(btn) && btn == MOUSE_BUTTON_LEFT)
        state.gizmoType = SCENE_OVERLAY_GIZMO_ROTATION;
    ui_pop();

    // scale gizmo
    iconRect = EditorIconAtlas::get_icon_rect(EditorIcon::LinearScale);
    ui_push_image(iconAtlas, iconSize, iconSize, 0xFFFFFFFF, &iconRect);
    ui_top_user(this);
    ui_top_draw([](UIWidget widget, ScreenRenderComponent renderer, void* user) {
        auto* obj = (ViewportWindowObj*)user;
        UITheme theme = obj->ctx.get_theme().get_ui_theme();
        if (obj->state.gizmoType == SCENE_OVERLAY_GIZMO_SCALE)
            renderer.draw_rect(widget.get_rect(), theme.get_selection_color());
        UIImageWidget::on_draw(widget, renderer);
    });
    if (ui_top_mouse_down(btn) && btn == MOUSE_BUTTON_LEFT)
        state.gizmoType = SCENE_OVERLAY_GIZMO_SCALE;
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

void ViewportWindowObj::viewport_editor_imgui(ViewportState& state)
{
    MouseButton btn;
    Vec2 pos;
    KeyCode key;
    bool begin;

    if (ui_top_key_down(key))
    {
        switch (key)
        {
        case KEY_CODE_1:
            state.gizmoType = SCENE_OVERLAY_GIZMO_TRANSLATION;
            break;
        case KEY_CODE_2:
            state.gizmoType = SCENE_OVERLAY_GIZMO_ROTATION;
            break;
        case KEY_CODE_3:
            state.gizmoType = SCENE_OVERLAY_GIZMO_SCALE;
            break;
        default:
            break;
        }
    }

    if (mode == VIEWPORT_MODE_2D)
    {
        viewport2D.imgui(state);
    }
    else
    {
        viewport3D.imgui(state);
    }
}

void ViewportWindowObj::viewport_scene_imgui()
{
    MouseButton btn;
    Vec2 pos;

    if (!root.get_mouse_pos(pos))
        return;

    Scene scene = ctx.get_scene();
    pos = Vec2(pos.x, pos.y - VIEWPORT_TOOLBAR_HEIGHT);
    WindowMouseMotionEvent motionE(0, pos.x, pos.y);
    scene.input_screen_ui(&motionE);

    if (ui_top_mouse_down(btn))
    {
        WindowMouseDownEvent mouseDownE(0, btn);
        scene.input_screen_ui(&mouseDownE);
    }
}

void ViewportWindowObj::on_editor_event(const EditorEvent* event, void* user)
{
    auto& self = *(ViewportWindowObj*)user;

    if (event->type != EDITOR_EVENT_TYPE_NOTIFY_COMPONENT_SELECTION)
        return;

    const auto* selectionEvent = static_cast<const EditorNotifyComponentSelectionEvent*>(event);

    // clear gizmo
    if (selectionEvent->component == 0)
    {
        self.state.gizmoSubjectSUID = 0;
        return;
    }

    Scene::Component subject = self.ctx.get_component(selectionEvent->component);
    LD_ASSERT(subject);

    Mat4 worldMat4;
    if (subject.get_world_mat4(worldMat4))
    {
        self.state.gizmoSubjectSUID = selectionEvent->component;
    }
    else
    {
        // selected component type has no transforms for gizmo
        self.state.gizmoSubjectSUID = 0;
    }
}

//
// Public API
//

EditorWindow ViewportWindow::create(const EditorWindowInfo& windowI)
{
    LD_PROFILE_SCOPE;

    ViewportWindowObj* obj = heap_new<ViewportWindowObj>(MEMORY_USAGE_UI);
    obj->ctx = windowI.ctx;
    obj->space = windowI.space;
    obj->root = obj->space.create_window(obj->space.get_root_id(), {}, {}, nullptr);
    obj->root.layout();
    obj->state.viewportExtent = obj->root.get_size();
    obj->state.sceneExtent = Vec2(obj->state.viewportExtent.x, obj->state.viewportExtent.y - VIEWPORT_TOOLBAR_HEIGHT);
    obj->state.gizmoType = SCENE_OVERLAY_GIZMO_TRANSLATION;

    obj->viewport2D.create(obj->ctx, obj->state.sceneExtent);
    obj->viewport3D.create(obj->ctx, obj->state.sceneExtent);

    obj->ctx.resize_scene(obj->state.sceneExtent);
    obj->ctx.add_observer(&ViewportWindowObj::on_editor_event, obj);

    return {obj};
}

void ViewportWindow::destroy(EditorWindow viewport)
{
    LD_PROFILE_SCOPE;

    LD_ASSERT(viewport && viewport.get_type() == EDITOR_WINDOW_VIEWPORT);

    auto* obj = static_cast<ViewportWindowObj*>(viewport.unwrap());

    obj->viewport2D.destroy();
    obj->viewport3D.destroy();

    heap_delete<ViewportWindowObj>(obj);
}

Camera ViewportWindow::get_editor_camera()
{
    if (mObj->mode != VIEWPORT_MODE_3D)
        return {};

    return mObj->viewport3D.get_camera();
}

Camera2D ViewportWindow::get_editor_camera_2d()
{
    if (mObj->mode != VIEWPORT_MODE_2D)
        return {};

    return mObj->viewport2D.get_camera_2d();
}

Vec2 ViewportWindow::get_size()
{
    return mObj->state.viewportExtent;
}

Vec2 ViewportWindow::get_scene_size()
{
    return mObj->state.sceneExtent;
}

bool ViewportWindow::get_mouse_pos(Vec2& mousePos)
{
    if (mObj->state.sceneMousePos.x < 0.0f || mObj->state.sceneMousePos.y < 0.0f)
        return false;

    mousePos = mObj->state.sceneMousePos;
    return true;
}

void ViewportWindow::get_gizmo_3d_state(SceneOverlayGizmo& gizmoType, Vec3& gizmoCenter, float& gizmoScale, RenderSystemSceneGizmoColor& gizmoColor)
{
    if (mObj->mode != VIEWPORT_MODE_3D || !mObj->state.gizmoSubjectSUID)
    {
        gizmoType = SCENE_OVERLAY_GIZMO_NONE;
        return;
    }

    gizmoType = mObj->state.gizmoType;

    GizmoAxis axis;
    GizmoPlane plane;
    GizmoControl control = mObj->viewport3D.get_gizmo_state(gizmoCenter, gizmoScale, axis, plane);

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
        switch (mObj->state.hoverGizmoID)
        {
        case SCENE_OVERLAY_GIZMO_ID_AXIS_X:
        case SCENE_OVERLAY_GIZMO_ID_AXIS_Y:
        case SCENE_OVERLAY_GIZMO_ID_AXIS_Z:
            gizmoAxisColors[(int)mObj->state.hoverGizmoID - SCENE_OVERLAY_GIZMO_ID_AXIS_X] = highlightColor;
            break;
        case SCENE_OVERLAY_GIZMO_ID_PLANE_XY:
        case SCENE_OVERLAY_GIZMO_ID_PLANE_XZ:
        case SCENE_OVERLAY_GIZMO_ID_PLANE_YZ:
            gizmoPlaneColors[(int)mObj->state.hoverGizmoID - SCENE_OVERLAY_GIZMO_ID_PLANE_XY] = highlightColor;
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
    mObj->state.hoverGizmoID = (SceneOverlayGizmoID)0;
    mObj->state.hoverRUID = (RUID)0;

    if ((int)gizmoID != 0)
    {
        mObj->state.hoverGizmoID = gizmoID;
        mObj->state.hoverRUID = (RUID)0;
    }
    else if ((int)ruid != 0)
    {
        mObj->state.hoverGizmoID = (SceneOverlayGizmoID)0;
        mObj->state.hoverRUID = ruid;
    }
}

} // namespace LD
