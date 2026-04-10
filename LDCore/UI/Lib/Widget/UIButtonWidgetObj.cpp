#include <Ludens/UI/Widget/UIButtonWidget.h>

#include "../UIContextObj.h"
#include "../UIWidgetObj.h"
#include "UIButtonWidgetObj.h"

namespace LD {

void UIButtonWidgetObj::startup(UIWidgetObj* obj, void* storage)
{
    UIButtonWidgetObj& self = obj->as.button;
    new (&self) UIButtonWidgetObj();

    self.base = obj;
    self.storage = (UIButtonStorage*)storage;

    if (!self.storage)
    {
        obj->flags |= UI_WIDGET_FLAG_LOCAL_STORAGE_BIT;
        self.storage = &self.local;
        self.storage->font = obj->ctx()->fontDefault;
    }

    UIButtonWidget handle{obj};
}

void UIButtonWidgetObj::cleanup(UIWidgetObj* base)
{
    UIButtonWidgetObj& self = base->as.button;

    (&self)->~UIButtonWidgetObj();
}

bool UIButtonWidgetObj::on_event(UIWidgetObj* obj, const UIEvent& event)
{
    UIButtonWidgetObj& self = obj->as.button;

    // TODO: click semantics, usually when MOUSE_UP event is still within the button rect.
    if (event.type == UI_EVENT_MOUSE_DOWN && self.storage->isEnabled && self.onClick)
    {
        self.onClick(UIWidget(obj), event.mouse.button, obj->user);
        return true;
    }

    return false;
}

void UIButtonWidgetObj::on_draw(UIWidgetObj* obj, ScreenRenderComponent renderer)
{
    UIContextObj* ctx = obj->ctx();
    UIButtonWidgetObj& self = obj->as.button;
    UIButtonStorage* storage = self.storage;
    UIWidget widget(obj);
    UITheme theme = widget.get_theme();
    Color bgColor = theme.get_selection_color();
    Color fgColor = theme.get_on_surface_color();
    const Rect& rect = obj->layout.rect;
    float wrapWidth = rect.w;
    float fontSize = rect.h * 0.8f;

    if (!storage->font)
        storage->font = obj->ctx()->fontDefault;

    if (storage->isEnabled)
    {
        if (widget.is_pressed())
            bgColor = Color::darken(bgColor, 0.05f);
        else if (widget.is_hovered())
            bgColor = Color::lift(bgColor, 0.07f);
    }
    else
    {
        bgColor.set_alpha(0.2f);
        fgColor.set_alpha(0.2f);
    }

    if (!storage->transparentBG)
        renderer.draw_rect(rect, bgColor);

    if (!storage->text.empty())
        renderer.draw_text_centered(storage->font.font_atlas(), storage->font.image(), fontSize, rect.get_pos(), storage->text.c_str(), fgColor, wrapWidth);
}

UIButtonStorage* UIButtonWidget::get_storage()
{
    return mObj->as.button.storage;
}

void UIButtonWidget::set_storage(UIButtonStorage* storage)
{
    mObj->as.button.storage = storage;
}

void UIButtonWidget::set_on_click(UIButtonOnClick onClick)
{
    mObj->as.button.onClick = onClick;
}

} // namespace LD