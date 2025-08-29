#pragma once

#include <Ludens/Gizmo/Gizmo.h>
#include <Ludens/RenderComponent/SceneOverlayComponent.h>
#include <Ludens/UI/UIContext.h>

namespace LD {

struct ViewportToolbar
{
    UIWindow window;
    UIButtonWidget transformBtn;
    UIButtonWidget rotateBtn;
    UIButtonWidget scaleBtn;
    SceneOverlayGizmo* gizmoType;

    void startup(UIContext ctx, float width, SceneOverlayGizmo* gizmoType);

    static void on_draw(UIWidget widget, ScreenRenderComponent renderer);
    static void on_press_transform_btn(UIButtonWidget w, MouseButton btn, void* user);
    static void on_press_rotate_btn(UIButtonWidget w, MouseButton btn, void* user);
    static void on_press_scale_btn(UIButtonWidget w, MouseButton btn, void* user);
};

} // namespace LD