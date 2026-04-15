#include "UIScrollBarWidgetObj.h"
#include "../UIContextObj.h"
#include "../UIWidgetObj.h"

namespace LD {

void UIScrollBarWidgetObj::on_mouse_drag(const UIEvent& event)
{
    if (event.type != UI_EVENT_MOUSE_DRAG || event.drag.button != MOUSE_BUTTON_LEFT)
        return;

    UIScrollBarData& data = get_data();
    Rect rect = get_rect();
    Vec2 localPos = event.drag.position - rect.get_pos();

    if (data.axis == UI_AXIS_X)
        data.mRatio = std::clamp(localPos.x / rect.w, 0.0f, 1.0f);
    else
        data.mRatio = std::clamp(localPos.y / rect.h, 0.0f, 1.0f);
}

void UIScrollBarWidgetObj::startup(UIWidgetObj* obj)
{
    UIScrollBarWidgetObj& self = obj->U->scrollBar;
    new (&self) UIScrollBarWidgetObj();
    self.connect(obj);
}

void UIScrollBarWidgetObj::cleanup(UIWidgetObj* obj)
{
    UIScrollBarWidgetObj& self = obj->U->scrollBar;

    (&self)->~UIScrollBarWidgetObj();
}

bool UIScrollBarWidgetObj::on_event(UIWidgetObj* obj, const UIEvent& event)
{
    UIScrollBarWidgetObj& self = obj->U->scrollBar;

    switch (event.type)
    {
    case UI_EVENT_MOUSE_DOWN:
        return true;
    case UI_EVENT_MOUSE_DRAG:
        self.on_mouse_drag(event);
        return true;
    default:
        break;
    }

    return false;
}

void UIScrollBarWidgetObj::on_draw(UIWidgetObj* obj, ScreenRenderComponent renderer)
{
    UIScrollBarWidgetObj& self = obj->U->scrollBar;
    const UIScrollBarData& data = self.get_data();
    Rect rect = self.get_rect();

    renderer.draw_rect(rect, data.bgColor);

    const float radius = 1.0f;

    if (data.axis == UI_AXIS_X)
    {
        rect = Rect::map_normalized(rect, Rect(data.mRatio, 0.0f, 0.1f, 1.0f));
        renderer.draw_rect_rounded(rect, data.barColor, radius);
    }
    else
    {
        rect = Rect::map_normalized(rect, Rect(0.0f, data.mRatio, 1.0f, 0.1f));
        renderer.draw_rect_rounded(rect, data.barColor, radius);
    }
}

} // namespace LD