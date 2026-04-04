#include <Ludens/UI/Widget/UIToggleWidget.h>

#include "../UIWidgetObj.h"
#include "UIToggleWidgetObj.h"

namespace LD {

void UIToggleWidgetObj::startup(UIWidgetObj* obj, void* storage)
{
    UIToggleWidgetObj& self = obj->as.toggle;
    new (&self) UIToggleWidgetObj();

    self.base = obj;
    self.storage = (UIToggleStorage*)storage;

    if (!self.storage)
    {
        obj->flags |= UI_WIDGET_FLAG_LOCAL_STORAGE_BIT;
        self.storage = &self.local;
    }

    obj->as.toggle.anim.reset(1.0f);
}

void UIToggleWidgetObj::cleanup(UIWidgetObj* obj)
{
    UIToggleWidgetObj& self = obj->as.toggle;
    (&self)->~UIToggleWidgetObj();
}

bool UIToggleWidgetObj::on_event(UIWidgetObj* obj, const UIEvent& event)
{
    UIToggleWidgetObj& self = obj->as.toggle;

    if (event.type == UI_EVENT_MOUSE_DOWN)
    {
        self.storage->state = !self.storage->state;
        self.anim.set(0.32f);

        if (self.onToggle)
            self.onToggle({obj}, self.storage->state, obj->user);

        return true;
    }

    return false;
}

void UIToggleWidgetObj::on_update(UIWidgetObj* obj, float delta)
{
    UIToggleWidgetObj& self = obj->as.toggle;

    // drive toggle animation
    self.anim.update(delta);
}

void UIToggleWidgetObj::on_draw(UIWidgetObj* obj, ScreenRenderComponent renderer)
{
    UIWidget widget(obj);
    UITheme theme = widget.get_theme();
    UIToggleWidgetObj& self = obj->as.toggle;
    Rect rect = widget.get_rect();

    renderer.draw_rect(rect, theme.get_background_color());

    rect.w /= 2.0f;

    // animate position
    float ratio = self.anim.get();
    if (!self.storage->state)
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

UIToggleStorage* UIToggleWidget::get_storage()
{
    return mObj->as.toggle.storage;
}

void UIToggleWidget::set_on_toggle(UIToggleOnToggle onToggle)
{
    mObj->as.toggle.onToggle = onToggle;
}

} // namespace LD