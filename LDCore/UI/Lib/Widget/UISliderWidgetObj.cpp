#include <Ludens/UI/Widget/UISliderWidget.h>

#include "../UIWidgetObj.h"

namespace LD {

void UISliderWidgetObj::startup(UIWidgetObj* obj)
{
    UISliderWidgetObj& self = obj->U->slider;
    new (&self) UISliderWidgetObj();

    self.base = obj;

    if (!obj->data)
    {
        obj->flags |= UI_WIDGET_FLAG_LOCAL_STORAGE_BIT;
        obj->data = &self.local;
    }
}

void UISliderWidgetObj::cleanup(UIWidgetObj* obj)
{
    UISliderWidgetObj& self = obj->U->slider;

    (&self)->~UISliderWidgetObj();
}

bool UISliderWidgetObj::on_event(UIWidgetObj* obj, const UIEvent& event)
{
    UISliderWidgetObj& self = obj->U->slider;
    UISliderData& data = *(UISliderData*)obj->data;

    if (event.type == UI_EVENT_MOUSE_DOWN)
        return true;

    if (event.type != UI_EVENT_MOUSE_DRAG)
        return false;

    Rect rect = obj->L->rect;
    data.mRatio = std::clamp(((float)event.drag.position.x - rect.x) / rect.w, 0.0f, 1.0f);

    return true;
}

void UISliderWidgetObj::on_draw(UIWidgetObj* obj, ScreenRenderComponent renderer)
{
    const UITheme& theme = obj->theme;
    UISliderWidgetObj& self = obj->U->slider;
    UISliderData& data = *(UISliderData*)obj->data;
    UIWidget widget(obj);
    Rect rect = obj->L->rect;

    float sliderw = rect.w * 0.1f;
    renderer.draw_rect(rect, theme.get_background_color());

    Color color = theme.get_selection_color();
    if (widget.is_pressed())
        color = Color::darken(color, 0.05f);
    if (widget.is_hovered())
        color = Color::lift(color, 0.07f);

    rect.w = sliderw;
    rect.x += data.mRatio * sliderw * 9.0f;
    renderer.draw_rect(rect, color);
}

float UISliderWidget::get_value()
{
    return mObj->U->slider.get_value();
}

} // namespace LD