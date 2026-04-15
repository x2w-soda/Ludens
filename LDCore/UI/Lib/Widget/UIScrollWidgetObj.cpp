#pragma once

#include "UIScrollWidgetObj.h"
#include "../UIWidgetObj.h"

namespace LD {

UILayoutInfo UIScrollWidgetObj::default_layout()
{
    UILayoutInfo layoutI(UISize::grow(), UISize::grow());
    layoutI.childAxis = UI_AXIS_Y;

    return layoutI;
}

void UIScrollWidgetObj::startup(UIWidgetObj* obj)
{
    UIScrollWidgetObj& self = obj->U->scroll;
    new (&self) UIScrollWidgetObj();
    self.connect(obj);

    obj->flags |= UI_WIDGET_FLAG_DRAW_WITH_SCISSOR_BIT;
}

void UIScrollWidgetObj::cleanup(UIWidgetObj* obj)
{
    UIScrollWidgetObj& self = obj->U->scroll;

    (&self)->~UIScrollWidgetObj();
}

bool UIScrollWidgetObj::on_event(UIWidgetObj* obj, const UIEvent& event)
{
    UIScrollWidgetObj& self = obj->U->scroll;
    UIScrollData& data = self.get_data();

    const float sensitivity = 20.0f;
    const float animDuration = 0.14f;

    if (event.type != UI_EVENT_SCROLL)
        return false;

    const Vec2& offset = event.scroll.offset;

    if (offset.x != 0.0f)
    {
        float offsetDstXRaw = data.mOffsetDst.x + offset.x * sensitivity;
        data.mOffsetSpeed.x = (offsetDstXRaw - obj->L->childOffset.x) / animDuration;
        self.set_offset_dst_x(offsetDstXRaw, false);
    }

    if (offset.y != 0.0f)
    {
        float offsetDstYRaw = data.mOffsetDst.y + offset.y * sensitivity;
        data.mOffsetSpeed.y = (offsetDstYRaw - obj->L->childOffset.y) / animDuration;
        self.set_offset_dst_y(offsetDstYRaw, false);
    }

    return true;
}

void UIScrollWidgetObj::set_offset_dst_x(float dstX, bool snap)
{
    UIScrollData& data = get_data();

    data.mOffsetDst.x = std::clamp(dstX, data.mMinScrollOffset.x, 0.0f);

    if (snap)
    {
        data.mOffsetSpeed = {};
        data.mSnapToDst = true;
    }
}

void UIScrollWidgetObj::set_offset_dst_y(float dstY, bool snap)
{
    UIScrollData& data = get_data();

    data.mOffsetDst.y = std::clamp(dstY, data.mMinScrollOffset.y, 0.0f);

    if (snap)
    {
        data.mOffsetSpeed = {};
        data.mSnapToDst = true;
    }
}

void UIScrollWidgetObj::on_update(UIWidgetObj* obj, float delta)
{
    UIScrollWidgetObj& self = obj->U->scroll;
    UIScrollData& data = self.get_data();
    Rect rect = self.get_rect();

    if (data.mSnapToDst)
    {
        data.mSnapToDst = false;
        data.mOffsetSpeed = {};
        obj->L->childOffset = data.mOffsetDst;
    }

    Vec2 childExtent = obj->get_child_rect_union().get_size();
    data.mMinScrollOffset.x = std::min(-childExtent.x, 0.0f);
    data.mMinScrollOffset.y = std::min(-childExtent.y, 0.0f);

    if (data.mOffsetSpeed.x != 0.0f)
    {
        obj->L->childOffset.x += data.mOffsetSpeed.x * delta;

        if ((data.mOffsetSpeed.x > 0.0f && obj->L->childOffset.x > data.mOffsetDst.x) ||
            (data.mOffsetSpeed.x < 0.0f && obj->L->childOffset.x < data.mOffsetDst.x))
        {
            obj->L->childOffset.x = data.mOffsetDst.x;
            data.mOffsetSpeed.x = 0.0f;
        }
    }

    if (data.mOffsetSpeed.y != 0.0f)
    {
        obj->L->childOffset.y += data.mOffsetSpeed.y * delta;

        if ((data.mOffsetSpeed.y > 0.0f && obj->L->childOffset.y > data.mOffsetDst.y) ||
            (data.mOffsetSpeed.y < 0.0f && obj->L->childOffset.y < data.mOffsetDst.y))
        {
            obj->L->childOffset.y = data.mOffsetDst.y;
            data.mOffsetSpeed.y = 0.0f;
        }
    }
}

void UIScrollWidgetObj::on_draw(UIWidgetObj* obj, ScreenRenderComponent renderer)
{
    UIScrollWidgetObj& self = obj->U->scroll;
    UIScrollData& data = self.get_data();

    renderer.draw_rect(self.get_rect(), data.bgColor);
}

void UIScrollWidget::set_scroll_offset_x(float offset)
{
    mObj->U->scroll.set_offset_dst_x(offset, false);
}

void UIScrollWidget::set_scroll_offset_x_normalized(float ratio)
{
    UIScrollWidgetObj& self = mObj->U->scroll;
    Vec2 minScrollOffset = self.get_data().get_min_scroll_offset();

    ratio = 1.0f - std::clamp(ratio, 0.0f, 1.0f);

    self.set_offset_dst_x(std::lerp(minScrollOffset.x, 0.0f, ratio), true);
}

float UIScrollWidget::get_scroll_offset_x_normalized()
{
    UIScrollWidgetObj& self = mObj->U->scroll;
    Vec2 minScrollOffset = self.get_data().get_min_scroll_offset();

    if (is_zero_epsilon(minScrollOffset.x))
        return 1.0f;

    return std::clamp(mObj->L->childOffset.x / minScrollOffset.x, 0.0f, 1.0f);
}

void UIScrollWidget::set_scroll_offset_y(float offset)
{
    mObj->U->scroll.set_offset_dst_y(offset, false);
}

void UIScrollWidget::set_scroll_offset_y_normalized(float ratio)
{
    UIScrollWidgetObj& self = mObj->U->scroll;
    Vec2 minScrollOffset = self.get_data().get_min_scroll_offset();

    ratio = 1.0f - std::clamp(ratio, 0.0f, 1.0f);

    mObj->U->scroll.set_offset_dst_y(std::lerp(minScrollOffset.y, 0.0f, ratio), true);
}

float UIScrollWidget::get_scroll_offset_y_normalized()
{
    UIScrollWidgetObj& self = mObj->U->scroll;
    Vec2 minScrollOffset = self.get_data().get_min_scroll_offset();

    if (is_zero_epsilon(minScrollOffset.y))
        return 1.0f;

    return std::clamp(mObj->L->childOffset.y / minScrollOffset.y, 0.0f, 1.0f);
}

} // namespace LD