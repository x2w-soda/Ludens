#include "ViewportToolbar.h"
#include <LudensEditor/EditorContext/EditorIconAtlas.h>

namespace LD {

void ViewportToolbar::startup(EditorContext eCtx, UIContext uiCtx, float width, SceneOverlayGizmo* gizmoType)
{
    this->eCtx = eCtx;
    this->gizmoType = gizmoType;
    isPlayIcon = true;

    UITheme theme = uiCtx.get_theme();
    const float buttonSize = 26;

    UILayoutInfo layoutI{};
    layoutI.sizeX = UISize::fixed(width);
    layoutI.sizeY = UISize::fixed(buttonSize);
    layoutI.childAxis = UI_AXIS_X;
    UIWindowInfo windowI{};
    windowI.defaultMouseControls = false;
    windowI.name = "ViewportToolbar";
    window = uiCtx.add_window(layoutI, windowI, this);
    window.set_on_draw(&ViewportToolbar::on_draw);

    layoutI.sizeX = UISize::fixed(buttonSize);
    layoutI.sizeY = UISize::fixed(buttonSize);
    Rect iconRect = EditorIconAtlas::get_icon_rect(EditorIcon::Transform);
    UIImageWidgetInfo imageWI{};
    imageWI.image = eCtx.get_editor_icon_atlas();
    imageWI.rect = &iconRect;
    transformIcon = window.node().add_image(layoutI, imageWI, this);
    transformIcon.set_on_mouse(&ViewportToolbar::on_press_transform_icon);

    iconRect = EditorIconAtlas::get_icon_rect(EditorIcon::Refresh);
    rotateIcon = window.node().add_image(layoutI, imageWI, this);
    rotateIcon.set_on_mouse(&ViewportToolbar::on_press_rotate_icon);

    iconRect = EditorIconAtlas::get_icon_rect(EditorIcon::LinearScale);
    scaleIcon = window.node().add_image(layoutI, imageWI, this);
    scaleIcon.set_on_mouse(&ViewportToolbar::on_press_scale_icon);

    iconRect = EditorIconAtlas::get_icon_rect(EditorIcon::PlayArrow);
    playIcon = window.node().add_image(layoutI, imageWI, this);
    playIcon.set_on_mouse(&ViewportToolbar::on_press_play_stop_icon);
    playIcon.set_on_hover([](UIWidget, UIEvent) {});
    playIcon.set_on_draw([](UIWidget widget, ScreenRenderComponent renderer) {
        UITheme theme = widget.get_theme();
        if (widget.is_hovered())
            renderer.draw_rect(widget.get_rect(), theme.get_field_color());
        UIImageWidget::on_draw(widget, renderer);
    });

    display(true);
}

void ViewportToolbar::display(bool isPlayIcon)
{
    this->isPlayIcon = isPlayIcon;

    EditorTheme theme = eCtx.get_theme();

    if (isPlayIcon)
    {
        playIcon.set_image_rect(EditorIconAtlas::get_icon_rect(EditorIcon::PlayArrow));
        playIcon.set_image_tint(theme.get_play_button_color());
    }
    else
    {
        playIcon.set_image_rect(EditorIconAtlas::get_icon_rect(EditorIcon::Close));
        playIcon.set_image_tint(theme.get_stop_button_color());
    }
}

void ViewportToolbar::on_draw(UIWidget widget, ScreenRenderComponent renderer)
{
    ViewportToolbar& self = *(ViewportToolbar*)widget.get_user();
    Rect rect = widget.get_rect();
    UITheme theme = widget.get_theme();

    renderer.draw_rect(rect, theme.get_surface_color());

    Color activeBG = theme.get_selection_color();
    if (*self.gizmoType == SCENE_OVERLAY_GIZMO_TRANSLATION)
        renderer.draw_rect(self.transformIcon.get_rect(), activeBG);

    if (*self.gizmoType == SCENE_OVERLAY_GIZMO_ROTATION)
        renderer.draw_rect(self.rotateIcon.get_rect(), activeBG);

    if (*self.gizmoType == SCENE_OVERLAY_GIZMO_SCALE)
        renderer.draw_rect(self.scaleIcon.get_rect(), activeBG);
}

void ViewportToolbar::on_press_transform_icon(UIWidget widget, const Vec2& pos, MouseButton btn, UIEvent event)
{
    ViewportToolbar& self = *(ViewportToolbar*)widget.get_user();

    if (btn != MOUSE_BUTTON_LEFT || event != UI_MOUSE_DOWN)
        return;

    *self.gizmoType = SCENE_OVERLAY_GIZMO_TRANSLATION;
}

void ViewportToolbar::on_press_rotate_icon(UIWidget widget, const Vec2& pos, MouseButton btn, UIEvent event)
{
    ViewportToolbar& self = *(ViewportToolbar*)widget.get_user();

    if (btn != MOUSE_BUTTON_LEFT || event != UI_MOUSE_DOWN)
        return;

    *self.gizmoType = SCENE_OVERLAY_GIZMO_ROTATION;
}

void ViewportToolbar::on_press_scale_icon(UIWidget widget, const Vec2& pos, MouseButton btn, UIEvent event)
{
    ViewportToolbar& self = *(ViewportToolbar*)widget.get_user();

    if (btn != MOUSE_BUTTON_LEFT || event != UI_MOUSE_DOWN)
        return;

    *self.gizmoType = SCENE_OVERLAY_GIZMO_SCALE;
}

void ViewportToolbar::on_press_play_stop_icon(UIWidget widget, const Vec2& pos, MouseButton btn, UIEvent event)
{
    ViewportToolbar& self = *(ViewportToolbar*)widget.get_user();

    if (btn != MOUSE_BUTTON_LEFT || event != UI_MOUSE_DOWN)
        return;

    if (self.isPlayIcon)
        self.isRequestingPlay.set(true);
    else
        self.isRequestingStop.set(true);
}

} // namespace LD