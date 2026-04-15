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

    data.mRatio = 1.0f;

    if (data.axis == UI_AXIS_X)
    {
        float scrollableSize = rect.w - data.mBarRect.w;
        if (scrollableSize > 0.0f)
            data.mRatio = std::clamp((localPos.x - data.mBarRect.w / 2.0f) / scrollableSize, 0.0f, 1.0f);
    }
    else
    {
        float scrollableSize = rect.h - data.mBarRect.h;
        if (scrollableSize > 0.0f)
            data.mRatio = std::clamp((localPos.y - data.mBarRect.h / 2.0f) / scrollableSize, 0.0f, 1.0f);
    }
}

float UIScrollBarWidgetObj::get_bar_size_ratio()
{
    const UIScrollBarData& data = get_data();

    if (is_zero_epsilon(data.contentExtent))
        return 1.0f;

    return std::min(data.scrollExtent / data.contentExtent, 1.0f);
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

void UIScrollBarWidgetObj::on_update(UIWidgetObj* obj, float delta)
{
    UIScrollBarWidgetObj& self = obj->U->scrollBar;
    UIScrollBarData& data = self.get_data();
    Rect rect = self.get_rect();

    const float barSizeRatio = self.get_bar_size_ratio();

    if (data.axis == UI_AXIS_X)
    {
        data.mBarRect = Rect::scale_w(rect, barSizeRatio);
        data.mBarRect.x = rect.x + (rect.w - data.mBarRect.w) * data.mRatio;
    }
    else
    {
        data.mBarRect = Rect::scale_h(rect, barSizeRatio);
        data.mBarRect.y = rect.y + (rect.h - data.mBarRect.h) * data.mRatio;
    }
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

    const float radius = 0.5f;

    renderer.draw_rect_rounded(rect, data.bgColor, radius);

    if (self.get_bar_size_ratio() < 1.0f)
        renderer.draw_rect_rounded(data.mBarRect, data.barColor, radius);
}

} // namespace LD