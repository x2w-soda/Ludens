#include <Ludens/UI/Widget/UIToggleWidget.h>

#include "../UIWidgetObj.h"
#include "UIToggleWidgetObj.h"

namespace LD {

void UIToggleWidgetObj::startup(UIWidgetObj* obj)
{
    UIToggleWidgetObj& self = obj->U->toggle;
    new (&self) UIToggleWidgetObj();
    self.connect(obj);

    self.get_data().mAnim.reset(1.0f);
}

void UIToggleWidgetObj::cleanup(UIWidgetObj* obj)
{
    UIToggleWidgetObj& self = obj->U->toggle;

    (&self)->~UIToggleWidgetObj();
}

bool UIToggleWidgetObj::on_event(UIWidgetObj* obj, const UIEvent& event)
{
    UIToggleWidgetObj& self = obj->U->toggle;
    UIToggleData& data = self.get_data();

    if (event.type == UI_EVENT_MOUSE_DOWN)
    {
        data.state = !data.state;
        data.mAnim.set(0.32f);

        if (data.onToggle)
            data.onToggle({obj}, data.state, obj->user);

        return true;
    }

    return false;
}

void UIToggleWidgetObj::on_update(UIWidgetObj* obj, float delta)
{
    UIToggleWidgetObj& self = obj->U->toggle;
    UIToggleData& data = self.get_data();

    // drive toggle animation
    data.mAnim.update(delta);
}

void UIToggleWidgetObj::on_draw(UIWidgetObj* obj, ScreenRenderComponent renderer)
{
    UIWidget widget(obj);
    UITheme theme = widget.get_theme();
    UIToggleWidgetObj& self = obj->U->toggle;
    UIToggleData& data = self.get_data();
    Rect rect = widget.get_rect();

    renderer.draw_rect(rect, theme.get_background_color());

    rect.w /= 2.0f;

    // animate position
    float ratio = data.mAnim.get();
    if (!data.state)
        ratio = 1.0f - ratio;

    rect.x += rect.w * ratio;

    uint32_t color = theme.get_selection_color();
    renderer.draw_rect(rect, widget.get_state_color(color));
}

void UIToggleWidget::set_on_toggle(UIToggleOnToggle onToggle)
{
    UIToggleWidgetObj& self = mObj->U->toggle;

    self.get_data().onToggle = onToggle;
}

} // namespace LD