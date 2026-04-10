#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/UI/UILayer.h>
#include <LudensEditor/EditorContext/EditorContextDef.h>

namespace LD {

struct EditorUIMainInfo
{
    EditorContext ctx;
    Vec2 screenSize;
    float topBarHeight;
};

struct EditorUIMain : Handle<struct EditorUIMainObj>
{
    static EditorUIMain create(const EditorUIMainInfo& modalI);
    static void destroy(EditorUIMain modal);

    void pre_update(const EditorUpdateTick& tick);
    void update(const EditorUpdateTick& tick);
    void post_update();

    void set_layout(EditorUIMainLayout layout);
    void set_viewport_hover_id(SceneOverlayGizmoID gizmoID, RUID ruid);
    Camera get_viewport_camera();
    Camera2D get_viewport_camera_2d();
    Vec2 get_viewport_size();
    Vec2 get_viewport_scene_size();
    bool get_viewport_mouse_pos(Vec2& pickPos);
    RUID get_viewport_outline_ruid();
    void get_viewport_gizmo_state(SceneOverlayGizmo& gizmoType, Vec3& gizmoCenter, float& gizmoScale, RenderSystemSceneGizmoColor& gizmoColor);
};

} // namespace LD