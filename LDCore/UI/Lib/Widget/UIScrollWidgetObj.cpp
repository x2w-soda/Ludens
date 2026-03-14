#pragma once

#include "UIScrollWidgetObj.h"
#include "../UIWidgetObj.h"

namespace LD {

void UIScrollWidgetObj::startup(UIWidgetObj* obj, void* storage)
{
    UIScrollWidgetObj& self = obj->as.scroll;
    new (&self) UIScrollWidgetObj();

    self.base = obj;
    self.storage = (UIScrollStorage*)storage;
    self.offsetXDst = 0.0f;
    self.offsetXSpeed = 0.0f;
    self.offsetYDst = 0.0f;
    self.offsetYSpeed = 0.0f;
    obj->cb.onDraw = &UIScrollWidget::on_draw;
    obj->cb.onUpdate = &UIScrollWidget::on_update;
    obj->cb.onEvent = &UIScrollWidgetObj::on_event;
    obj->flags |= UI_WIDGET_FLAG_DRAW_WITH_SCISSOR_BIT;
}

void UIScrollWidgetObj::cleanup(UIWidgetObj* obj)
{
    UIScrollWidgetObj& self = obj->as.scroll;

    (&self)->~UIScrollWidgetObj();
}

bool UIScrollWidgetObj::on_event(UIWidget widget, const UIEvent& event)
{
    UIWidgetObj* base = (UIWidgetObj*)widget;
    UIScrollWidgetObj& self = base->as.scroll;

    const float sensitivity = 20.0f;
    const float animDuration = 0.14f;

    if (event.type != UI_EVENT_SCROLL)
        return false;

    const Vec2& offset = event.scroll.offset;

    if (offset.x != 0.0f)
    {
        self.offsetXDst += offset.x * sensitivity;
        self.offsetXSpeed = (self.offsetXDst - base->scrollOffset.x) / animDuration;

        if (self.offsetXDst > 0.0f)
            self.offsetXDst = 0.0f;
    }

    if (offset.y != 0.0f)
    {
        self.offsetYDst += offset.y * sensitivity;
        self.offsetYSpeed = (self.offsetYDst - base->scrollOffset.y) / animDuration;

        if (self.offsetYDst > 0.0f)
            self.offsetYDst = 0.0f;
    }

    return true;
}

void UIScrollWidget::set_scroll_offset_x(float offset)
{
    mObj->scrollOffset.x = offset;
    mObj->as.scroll.offsetXDst = offset;
    mObj->as.scroll.offsetXSpeed = 0.0f;
}

void UIScrollWidget::set_scroll_offset_y(float offset)
{
    mObj->scrollOffset.y = offset;
    mObj->as.scroll.offsetYDst = offset;
    mObj->as.scroll.offsetYSpeed = 0.0f;
}

void UIScrollWidget::on_update(UIWidget widget, float delta)
{
    UIWidgetObj* base = widget.unwrap();
    UIScrollWidgetObj& self = base->as.scroll;

    if (self.offsetXSpeed != 0.0f)
    {
        base->scrollOffset.x += self.offsetXSpeed * delta;

        if ((self.offsetXSpeed > 0.0f && base->scrollOffset.x > self.offsetXDst) ||
            (self.offsetXSpeed < 0.0f && base->scrollOffset.x < self.offsetXDst))
        {
            base->scrollOffset.x = self.offsetXDst;
            self.offsetXSpeed = 0.0f;
        }
    }

    if (self.offsetYSpeed != 0.0f)
    {
        base->scrollOffset.y += self.offsetYSpeed * delta;

        if ((self.offsetYSpeed > 0.0f && base->scrollOffset.y > self.offsetYDst) ||
            (self.offsetYSpeed < 0.0f && base->scrollOffset.y < self.offsetYDst))
        {
            base->scrollOffset.y = self.offsetYDst;
            self.offsetYSpeed = 0.0f;
        }
    }
}

void UIScrollWidget::on_draw(UIWidget widget, ScreenRenderComponent renderer)
{
    UIWidgetObj* obj = widget;
    UIScrollWidgetObj& self = obj->as.scroll;
    Rect rect = widget.get_rect();

    renderer.draw_rect(rect, self.storage->bgColor);
}

} // namespace LD