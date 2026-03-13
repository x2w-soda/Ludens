#include "UIToggleWidgetObj.h"
#include "../UIWidgetObj.h"

namespace LD {

bool UIToggleWidgetObj::on_event(UIWidget widget, const UIEvent& event)
{
    UIWidgetObj* obj = (UIWidgetObj*)widget;
    UIToggleWidgetObj& self = obj->as.toggle;

    if (event.type == UI_EVENT_MOUSE_DOWN)
    {
        self.state = !self.state;
        self.anim.set(0.32f);

        if (self.user_on_toggle)
            self.user_on_toggle({obj}, self.state, obj->user);

        return true;
    }

    return false;
}

void UIToggleWidgetObj::on_update(UIWidget widget, float delta)
{
    UIWidgetObj* obj = (UIWidgetObj*)widget;
    UIToggleWidgetObj& self = obj->as.toggle;

    // drive toggle animation
    self.anim.update(delta);
}

void UIToggleWidget::on_draw(UIWidget widget, ScreenRenderComponent renderer)
{
    UIWidgetObj* obj = (UIWidgetObj*)widget;
    UITheme theme = widget.get_theme();
    UIToggleWidgetObj& self = obj->as.toggle;
    Rect rect = widget.get_rect();

    renderer.draw_rect(rect, theme.get_background_color());

    rect.w /= 2.0f;

    // animate position
    float ratio = self.anim.get();
    if (!self.state)
        ratio = 1.0f - ratio;

    rect.x += rect.w * ratio;

    uint32_t color = theme.get_on_surface_color();
    if (widget.is_pressed())
    {
        color &= ~0xFF;
        color |= 200;
    }
    else if (widget.is_hovered())
    {
        color &= ~0xFF;
        color |= 234;
    }
    renderer.draw_rect(rect, color);
}

} // namespace LD