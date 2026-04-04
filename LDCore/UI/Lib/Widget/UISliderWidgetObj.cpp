#include <Ludens/UI/Widget/UISliderWidget.h>

#include "../UIWidgetObj.h"

namespace LD {

void UISliderWidgetObj::startup(UIWidgetObj* obj, void* storage)
{
    UISliderWidgetObj& self = obj->as.slider;
    new (&self) UISliderWidgetObj();

    self.base = obj;
    self.storage = (UISliderStorage*)storage;

    if (!self.storage)
    {
        obj->flags |= UI_WIDGET_FLAG_LOCAL_STORAGE_BIT;
        self.storage = &self.local;
    }
}

void UISliderWidgetObj::cleanup(UIWidgetObj* obj)
{
    UISliderWidgetObj& self = obj->as.slider;

    (&self)->~UISliderWidgetObj();
}

bool UISliderWidgetObj::on_event(UIWidgetObj* obj, const UIEvent& event)
{
    UISliderWidgetObj& self = obj->as.slider;
    UISliderStorage* storage = self.storage;

    if (event.type == UI_EVENT_MOUSE_DOWN)
        return true;

    if (event.type != UI_EVENT_MOUSE_DRAG)
        return false;

    Rect rect = obj->layout.rect;
    storage->ratio = std::clamp(((float)event.drag.position.x - rect.x) / rect.w, 0.0f, 1.0f);

    return true;
}

void UISliderWidgetObj::on_draw(UIWidgetObj* obj, ScreenRenderComponent renderer)
{
    const UITheme& theme = obj->theme;
    UISliderWidgetObj& self = obj->as.slider;
    UISliderStorage* storage = self.storage;
    UIWidget widget(obj);
    Rect rect = obj->layout.rect;

    float sliderw = rect.w * 0.1f;
    renderer.draw_rect(rect, theme.get_background_color());

    Color color = theme.get_selection_color();
    if (widget.is_pressed())
        color = Color::darken(color, 0.05f);
    if (widget.is_hovered())
        color = Color::lift(color, 0.07f);

    rect.w = sliderw;
    rect.x += storage->ratio * sliderw * 9.0f;
    renderer.draw_rect(rect, color);
}

UISliderStorage* UISliderWidget::get_storage()
{
    return mObj->as.slider.storage;
}

float UISliderWidget::get_value()
{
    return mObj->as.slider.get_value();
}

} // namespace LD