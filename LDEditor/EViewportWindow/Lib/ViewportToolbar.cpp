#include "ViewportToolbar.h"

namespace LD {

void ViewportToolbar::startup(UIContext ctx, float width, SceneOverlayGizmo* gizmoType)
{
    this->gizmoType = gizmoType;

    const float buttonSize = 26;

    UILayoutInfo layoutI{};
    layoutI.sizeX = UISize::fixed(width);
    layoutI.sizeY = UISize::fixed(buttonSize);
    layoutI.childAxis = UI_AXIS_X;
    UIWindowInfo windowI{};
    windowI.defaultMouseControls = false;
    windowI.name = "ViewportToolbar";
    window = ctx.add_window(layoutI, windowI, this);
    window.set_on_draw(&ViewportToolbar::on_draw);

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

void ViewportToolbar::on_draw(UIWidget widget, ScreenRenderComponent renderer)
{
    ViewportToolbar& self = *(ViewportToolbar*)widget.get_user();
    Rect rect = widget.get_rect();
    UITheme theme = widget.get_theme();

    renderer.draw_rect(rect, theme.get_surface_color());

    Color activeBG = 0x4D6490FF;
    if (*self.gizmoType == SCENE_OVERLAY_GIZMO_TRANSLATION)
        renderer.draw_rect(self.transformBtn.get_rect(), activeBG);

    if (*self.gizmoType == SCENE_OVERLAY_GIZMO_ROTATION)
        renderer.draw_rect(self.rotateBtn.get_rect(), activeBG);

    if (*self.gizmoType == SCENE_OVERLAY_GIZMO_SCALE)
        renderer.draw_rect(self.scaleBtn.get_rect(), activeBG);
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