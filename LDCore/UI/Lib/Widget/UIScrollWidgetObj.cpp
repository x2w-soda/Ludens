#pragma once

#include "UIScrollWidgetObj.h"
#include "../UIWidgetObj.h"

namespace LD {

void UIScrollWidgetObj::startup(UIWidgetObj* obj, void* storage)
{
    UIScrollWidgetObj& self = obj->as.scroll;
    new (&self) UIScrollWidgetObj();

    self.base = obj;
    self.offsetXDst = 0.0f;
    self.offsetXSpeed = 0.0f;
    self.offsetYDst = 0.0f;
    self.offsetYSpeed = 0.0f;
    self.storage = (UIScrollStorage*)storage;

    if (!self.storage)
    {
        obj->flags |= UI_WIDGET_FLAG_LOCAL_STORAGE_BIT;
        self.storage = &self.local;
    }

    obj->flags |= UI_WIDGET_FLAG_DRAW_WITH_SCISSOR_BIT;
}

void UIScrollWidgetObj::cleanup(UIWidgetObj* obj)
{
    UIScrollWidgetObj& self = obj->as.scroll;

    (&self)->~UIScrollWidgetObj();
}

void on_update(UIWidgetObj* obj, float delta);
void on_draw(UIWidgetObj* obj, ScreenRenderComponent renderer);

bool UIScrollWidgetObj::on_event(UIWidgetObj* obj, const UIEvent& event)
{
    UIScrollWidgetObj& self = obj->as.scroll;

    const float sensitivity = 20.0f;
    const float animDuration = 0.14f;

    if (event.type != UI_EVENT_SCROLL)
        return false;

    const Vec2& offset = event.scroll.offset;

    if (offset.x != 0.0f)
    {
        self.offsetXDst += offset.x * sensitivity;
        self.offsetXSpeed = (self.offsetXDst - obj->scrollOffset.x) / animDuration;

        if (self.offsetXDst > 0.0f)
            self.offsetXDst = 0.0f;
    }

    if (offset.y != 0.0f)
    {
        self.offsetYDst += offset.y * sensitivity;
        self.offsetYSpeed = (self.offsetYDst - obj->scrollOffset.y) / animDuration;

        if (self.offsetYDst > 0.0f)
            self.offsetYDst = 0.0f;
    }

    return true;
}

UIScrollStorage* UIScrollWidget::get_storage()
{
    return mObj->as.scroll.storage;
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

void UIScrollWidgetObj::on_update(UIWidgetObj* obj, float delta)
{
    UIScrollWidgetObj& self = obj->as.scroll;

    if (self.offsetXSpeed != 0.0f)
    {
        obj->scrollOffset.x += self.offsetXSpeed * delta;

        if ((self.offsetXSpeed > 0.0f && obj->scrollOffset.x > self.offsetXDst) ||
            (self.offsetXSpeed < 0.0f && obj->scrollOffset.x < self.offsetXDst))
        {
            obj->scrollOffset.x = self.offsetXDst;
            self.offsetXSpeed = 0.0f;
        }
    }

    if (self.offsetYSpeed != 0.0f)
    {
        obj->scrollOffset.y += self.offsetYSpeed * delta;

        if ((self.offsetYSpeed > 0.0f && obj->scrollOffset.y > self.offsetYDst) ||
            (self.offsetYSpeed < 0.0f && obj->scrollOffset.y < self.offsetYDst))
        {
            obj->scrollOffset.y = self.offsetYDst;
            self.offsetYSpeed = 0.0f;
        }
    }
}

void UIScrollWidgetObj::on_draw(UIWidgetObj* obj, ScreenRenderComponent renderer)
{
    UIScrollWidgetObj& self = obj->as.scroll;

    renderer.draw_rect(obj->layout.rect, self.storage->bgColor);
}

} // namespace LD