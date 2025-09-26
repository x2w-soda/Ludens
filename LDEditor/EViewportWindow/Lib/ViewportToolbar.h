#pragma once

#include <Ludens/Gizmo/Gizmo.h>
#include <Ludens/Header/Impulse.h>
#include <Ludens/RenderComponent/SceneOverlayComponent.h>
#include <Ludens/UI/UIContext.h>
#include <LudensEditor/EditorContext/EditorContext.h>

namespace LD {

struct ViewportToolbar
{
    EditorContext eCtx;
    UIWindow window;
    UIImageWidget transformIcon;
    UIImageWidget rotateIcon;
    UIImageWidget scaleIcon;
    UIImageWidget playIcon;
    SceneOverlayGizmo* gizmoType;
    Impulse isRequestingPlay;
    Impulse isRequestingStop;
    bool isPlayIcon;

    void startup(EditorContext eCtx, UIContext uiCtx, float width, SceneOverlayGizmo* gizmoType);

    void display(bool isPlayIcon);

    static void on_draw(UIWidget widget, ScreenRenderComponent renderer);
    static void on_press_transform_icon(UIWidget widget, const Vec2& pos, MouseButton btn, UIEvent event);
    static void on_press_rotate_icon(UIWidget widget, const Vec2& pos, MouseButton btn, UIEvent event);
    static void on_press_scale_icon(UIWidget widget, const Vec2& pos, MouseButton btn, UIEvent event);
    static void on_press_play_stop_icon(UIWidget widget, const Vec2& pos, MouseButton btn, UIEvent event);
};

} // namespace LD