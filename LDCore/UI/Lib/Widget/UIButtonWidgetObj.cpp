#include <Ludens/UI/Widget/UIButtonWidget.h>

#include "../UIContextObj.h"
#include "../UIWidgetObj.h"
#include "UIButtonWidgetObj.h"

namespace LD {

void UIButtonWidgetObj::set_on_click(UIButtonOnClick onClick)
{
    UIButtonData& data = get_data();

    data.mOnClick = onClick;
}

void UIButtonWidgetObj::startup(UIWidgetObj* obj)
{
    UIButtonWidgetObj& self = obj->U->button;
    new (&self) UIButtonWidgetObj();
    self.connect(obj);
}

void UIButtonWidgetObj::cleanup(UIWidgetObj* obj)
{
    UIButtonWidgetObj& self = obj->U->button;

    (&self)->~UIButtonWidgetObj();
}

bool UIButtonWidgetObj::on_event(UIWidgetObj* obj, const UIEvent& event)
{
    UIButtonWidgetObj& self = obj->U->button;
    UIButtonData& data = self.get_data();

    // TODO: click semantics, usually when MOUSE_UP event is still within the button rect.
    if (event.type == UI_EVENT_MOUSE_DOWN && data.isEnabled && data.mOnClick)
    {
        data.mOnClick(UIWidget(obj), event.mouse.button, obj->user);
        return true;
    }

    return false;
}

void UIButtonWidgetObj::on_draw(UIWidgetObj* obj, ScreenRenderComponent renderer)
{
    UIContextObj* ctx = obj->ctx();
    UIButtonWidgetObj& self = obj->U->button;
    UIButtonData& data = self.get_data();
    const Rect& rect = self.get_rect();
    UIWidget widget(obj);
    UITheme theme = widget.get_theme();
    Color bgColor = theme.get_selection_color();
    Color fgColor = theme.get_on_surface_color();
    float wrapWidth = rect.w;
    float fontSize = rect.h * 0.8f;

    if (!data.font)
        data.font = obj->ctx()->fontDefault;

    if (data.isEnabled)
    {
        bgColor = widget.get_state_color(bgColor);
    }
    else
    {
        bgColor.set_alpha(0.2f);
        fgColor.set_alpha(0.2f);
    }

    if (!data.transparentBG)
        renderer.draw_rect(rect, bgColor);

    if (!data.text.empty())
        renderer.draw_text_centered(data.font.font_atlas(), data.font.image(), fontSize, rect.get_pos(), data.text.c_str(), fgColor, wrapWidth);
}

void UIButtonWidget::set_on_click(UIButtonOnClick onClick)
{
    mObj->U->button.set_on_click(onClick);
}

} // namespace LD