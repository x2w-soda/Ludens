#include <Ludens/UI/UIWidget.h>

#include "../UIContextObj.h"
#include "../UIWidgetObj.h"
#include "UITextEditWidgetObj.h"

namespace LD {

void UITextEditWidgetObj::cleanup(UIWidgetObj* base)
{
    UITextEditWidgetObj& self = base->as.textEdit;

    if (self.buf)
    {
        TextBuffer<char>::destroy(self.buf);
        self.buf = {};
    }

    if (self.placeHolder)
    {
        heap_free((void*)self.placeHolder);
        self.placeHolder = nullptr;
    }
}

bool UITextEditWidgetObj::on_event(UIWidget widget, const UIEvent& event)
{
    UIWidgetObj* obj = widget.unwrap();
    auto& self = obj->as.textEdit;

    if (event.type == UI_EVENT_MOUSE_DOWN)
        return true; // consume event

    if (event.type != UI_EVENT_KEY_DOWN)
        return false;

    bool hasChanged = false;
    bool hasSubmitted = false;

    switch (self.domain)
    {
    case UI_TEXT_EDIT_DOMAIN_STRING:
        self.domain_string_on_key(event, hasChanged, hasSubmitted);
        break;
    case UI_TEXT_EDIT_DOMAIN_UINT:
        self.domain_uint_on_key(event, hasChanged, hasSubmitted);
        break;
    }

    std::string str = self.buf.to_string();
    View strView(str.data(), str.size());

    if (hasChanged && self.onChange)
        self.onChange((UITextEditWidget)widget, strView, obj->user);

    if (hasSubmitted && self.onSubmit)
        self.onSubmit((UITextEditWidget)widget, strView, obj->user);

    return true;
}

void UITextEditWidgetObj::domain_string_on_key(const UIEvent& event, bool& hasChanged, bool& hasSubmitted)
{
    LD_ASSERT(domain == UI_TEXT_EDIT_DOMAIN_STRING);

    const KeyCode code = event.key.code;
    const KeyMods mods = event.key.mods;

    if (KEY_CODE_A <= code && code <= KEY_CODE_Z)
    {
        char key = (char)code + 32;

        if (mods & KEY_MOD_SHIFT_BIT)
            key -= 32;

        buf.push_back(key);
        hasChanged = true;
    }
    else if (KEY_CODE_0 <= code && code <= KEY_CODE_9)
    {
        char key = (char)code - (char)KEY_CODE_0 + '0';

        buf.push_back(key);
        hasChanged = true;
    }
    else if (code == KEY_CODE_SPACE)
    {
        buf.push_back(' ');
        hasChanged = true;
    }
    else if (code == KEY_CODE_BACKSPACE && !buf.empty())
    {
        buf.pop_back();
        hasChanged = true;
    }
    else if (code == KEY_CODE_ENTER)
    {
        // this allows submission of empty text.
        hasSubmitted = true;
    }
}

void UITextEditWidgetObj::domain_uint_on_key(const UIEvent& event, bool& hasChanged, bool& hasSubmitted)
{
    LD_ASSERT(domain == UI_TEXT_EDIT_DOMAIN_UINT);

    const KeyCode code = event.key.code;
    const KeyMods mods = event.key.mods;

    if (KEY_CODE_0 <= code && code <= KEY_CODE_9)
    {
        char key = (char)code - (char)KEY_CODE_0 + '0';

        buf.push_back(key);
        hasChanged = true;
    }
    else if (code == KEY_CODE_BACKSPACE && !buf.empty())
    {
        buf.pop_back();
        hasChanged = true;
    }
    else if (code == KEY_CODE_ENTER && !buf.empty())
    {
        hasSubmitted = true;
    }
}

void UITextEditWidget::set_text(View text)
{
    auto& self = mObj->as.textEdit;

    self.buf.set_string(text);
}

void UITextEditWidget::set_domain(UITextEditDomain domain)
{
    auto& self = mObj->as.textEdit;

    if (self.domain != domain)
    {
        self.buf.clear();
        self.domain = domain;
    }
}

void UITextEditWidget::on_draw(UIWidget widget, ScreenRenderComponent renderer)
{
    UIWidgetObj* obj = widget.unwrap();
    auto& self = obj->as.textEdit;
    UIContextObj& ctx = *obj->ctx();
    const UITheme& theme = ctx.theme;
    std::string str;

    Rect rect = widget.get_rect();
    renderer.draw_rect(rect, theme.get_field_color());

    if (widget.is_focused())
        renderer.draw_rect_outline(rect, 1, theme.get_primary_color());
    else if (widget.is_hovered())
        renderer.draw_rect_outline(rect, 1, theme.get_surface_color());

    if (!self.buf.empty())
    {
        float wrapWidth = rect.w;
        str = self.buf.to_string();
        renderer.draw_text(ctx.fontAtlas, ctx.fontAtlasImage, self.fontSize, rect.get_pos(), str.c_str(), theme.get_on_surface_color(), wrapWidth);
    }
    else if (self.placeHolder)
    {
        // TODO: place holder text
    }
}

} // namespace LD