#include "../UIWidgetObj.h"

namespace LD {

bool UISliderWidgetObj::on_event(UIWidget widget, const UIEvent& event)
{
    UISliderWidgetObj& self = static_cast<UIWidgetObj*>(widget)->as.slider;

    if (event.type == UI_EVENT_MOUSE_DOWN)
        return true;

    if (event.type != UI_EVENT_MOUSE_DRAG)
        return false;

    Rect rect = widget.get_rect();
    self.ratio = std::clamp(((float)event.drag.position.x - rect.x) / rect.w, 0.0f, 1.0f);
    self.value = std::lerp(self.min, self.max, self.ratio);

    return true;
}

void UISliderWidget::on_draw(UIWidget widget, ScreenRenderComponent renderer)
{
    UIWidgetObj* obj = widget;
    const UITheme& theme = obj->theme;
    UISliderWidgetObj& self = obj->as.slider;
    Rect rect = widget.get_rect();

    float sliderw = rect.w * 0.1f;
    renderer.draw_rect(rect, theme.get_background_color());

    Color color = theme.get_selection_color();
    if (widget.is_pressed())
        color = Color::darken(color, 0.05f);
    if (widget.is_hovered())
        color = Color::lift(color, 0.07f);

    rect.w = sliderw;
    rect.x += self.ratio * sliderw * 9.0f;
    renderer.draw_rect(rect, color);
}

void UISliderWidget::set_value_range(float minValue, float maxValue)
{
    mObj->as.slider.min = minValue;
    mObj->as.slider.max = maxValue;
    mObj->as.slider.value = std::clamp(mObj->as.slider.value, minValue, maxValue);
}

float UISliderWidget::get_value()
{
    return mObj->as.slider.value;
}

float UISliderWidget::get_ratio()
{
    return mObj->as.slider.ratio;
}

} // namespace LD