#include "ViewportToolbar.h"

namespace LD {

void ViewportToolbar::startup(UIContext ctx, SceneOverlayGizmo* gizmoType)
{
    this->gizmoType = gizmoType;

    const float buttonSize = 30;

    UILayoutInfo layoutI{};
    layoutI.sizeX = UISize::fit();
    layoutI.sizeY = UISize::fixed(buttonSize);
    layoutI.childAxis = UI_AXIS_X;
    UIWindowInfo windowI{};
    windowI.defaultMouseControls = false;
    windowI.name = "ViewportToolbar";
    window = ctx.add_window(layoutI, windowI, this);

    layoutI.sizeX = UISize::fixed(buttonSize);
    layoutI.sizeY = UISize::fixed(buttonSize);
    UIButtonWidgetInfo buttonWI{};
    buttonWI.text = "T";
    buttonWI.textColor = 0xFFFFFFFF;
    buttonWI.on_press = &ViewportToolbar::on_press_transform_btn;
    buttonWI.transparentBG = true;
    transformBtn = window.node().add_button(layoutI, buttonWI, this);

    buttonWI.text = "R";
    buttonWI.on_press = &ViewportToolbar::on_press_rotate_btn;
    rotateBtn = window.node().add_button(layoutI, buttonWI, this);

    buttonWI.text = "S";
    buttonWI.on_press = &ViewportToolbar::on_press_scale_btn;
    scaleBtn = window.node().add_button(layoutI, buttonWI, this);
}

void ViewportToolbar::on_draw_overlay(ScreenRenderComponent renderer)
{
    Rect rect = window.get_rect();

    uint32_t sw, sh;
    renderer.get_screen_extent(sw, sh);

    RImage blurBG = renderer.get_sampled_image();
    Rect uv;
    uv.x = rect.x / sw;
    uv.y = rect.y / sh;
    uv.w = rect.w / sw;
    uv.h = rect.h / sh;
    renderer.draw_image_uv(rect, blurBG, uv, 0xFFFFFFFF);

    Color activeBG = 0xFFA500A0;

    if (*gizmoType == SCENE_OVERLAY_GIZMO_TRANSLATION)
        renderer.draw_rect(transformBtn.get_rect(), activeBG);
    transformBtn.on_draw(renderer);

    if (*gizmoType == SCENE_OVERLAY_GIZMO_ROTATION)
        renderer.draw_rect(rotateBtn.get_rect(), activeBG);
    rotateBtn.on_draw(renderer);

    if (*gizmoType == SCENE_OVERLAY_GIZMO_SCALE)
        renderer.draw_rect(scaleBtn.get_rect(), activeBG);
    scaleBtn.on_draw(renderer);
}

void ViewportToolbar::on_press_transform_btn(UIButtonWidget w, MouseButton btn, void* user)
{
    ViewportToolbar& self = *(ViewportToolbar*)user;

    *self.gizmoType = SCENE_OVERLAY_GIZMO_TRANSLATION;
}

void ViewportToolbar::on_press_rotate_btn(UIButtonWidget w, MouseButton btn, void* user)
{
    ViewportToolbar& self = *(ViewportToolbar*)user;

    *self.gizmoType = SCENE_OVERLAY_GIZMO_ROTATION;
}

void ViewportToolbar::on_press_scale_btn(UIButtonWidget w, MouseButton btn, void* user)
{
    ViewportToolbar& self = *(ViewportToolbar*)user;

    *self.gizmoType = SCENE_OVERLAY_GIZMO_SCALE;
}

} // namespace LD