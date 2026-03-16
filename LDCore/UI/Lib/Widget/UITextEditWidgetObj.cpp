#include <Ludens/UI/UIWidget.h>

#include "../UIContextObj.h"
#include "../UIWidgetMeta.h"
#include "../UIWidgetObj.h"
#include "UITextEditWidgetObj.h"

namespace LD {

void UITextEditWidgetObj::startup(UIWidgetObj* obj, void* storage)
{
    UITextEditWidgetObj& self = obj->as.textEdit;
    new (&self) UITextEditWidgetObj();
    self.storage = (UITextEditStorage*)storage;

    if (!self.storage)
    {
        obj->flags |= UI_WIDGET_FLAG_LOCAL_STORAGE_BIT;
        self.storage = &self.local;
    }

    obj->flags |= UI_WIDGET_FLAG_FOCUSABLE_BIT;
    obj->cb.onEvent = &UITextEditWidgetObj::on_event;
    obj->cb.onDraw = &UITextEditWidget::on_draw;
}

void UITextEditWidgetObj::cleanup(UIWidgetObj* base)
{
    UITextEditWidgetObj* obj = &base->as.textEdit;

    obj->~UITextEditWidgetObj();
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

    switch (self.storage->domain)
    {
    case UI_TEXT_EDIT_DOMAIN_STRING:
        self.domain_string_on_key(event, hasChanged, hasSubmitted);
        break;
    case UI_TEXT_EDIT_DOMAIN_UINT:
        self.domain_uint_on_key(event, hasChanged, hasSubmitted);
        break;
    }

    std::string str = self.storage->buf.to_string();
    View strView(str.data(), str.size());

    if (hasChanged && self.onChange)
        self.onChange((UITextEditWidget)widget, strView, obj->user);

    if (hasSubmitted && self.onSubmit)
        self.onSubmit((UITextEditWidget)widget, strView, obj->user);

    return true;
}

void UITextEditWidgetObj::domain_string_on_key(const UIEvent& event, bool& hasChanged, bool& hasSubmitted)
{
    LD_ASSERT(storage->domain == UI_TEXT_EDIT_DOMAIN_STRING);

    const KeyCode code = event.key.code;
    const KeyMods mods = event.key.mods;

    if (KEY_CODE_A <= code && code <= KEY_CODE_Z)
    {
        char key = (char)code + 32;

        if (mods & KEY_MOD_SHIFT_BIT)
            key -= 32;

        storage->buf.push_back(key);
        hasChanged = true;
    }
    else if (KEY_CODE_0 <= code && code <= KEY_CODE_9)
    {
        char key = (char)code - (char)KEY_CODE_0 + '0';

        storage->buf.push_back(key);
        hasChanged = true;
    }
    else if (code == KEY_CODE_SPACE)
    {
        storage->buf.push_back(' ');
        hasChanged = true;
    }
    else if (code == KEY_CODE_BACKSPACE && !storage->buf.empty())
    {
        storage->buf.pop_back();
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
    LD_ASSERT(storage->domain == UI_TEXT_EDIT_DOMAIN_UINT);

    const KeyCode code = event.key.code;
    const KeyMods mods = event.key.mods;

    if (KEY_CODE_0 <= code && code <= KEY_CODE_9)
    {
        char key = (char)code - (char)KEY_CODE_0 + '0';

        storage->buf.push_back(key);
        hasChanged = true;
    }
    else if (code == KEY_CODE_BACKSPACE && !storage->buf.empty())
    {
        storage->buf.pop_back();
        hasChanged = true;
    }
    else if (code == KEY_CODE_ENTER && !storage->buf.empty())
    {
        hasSubmitted = true;
    }
}

UITextEditStorage::UITextEditStorage()
{
    buf = TextBuffer<char>::create();
}

UITextEditStorage::UITextEditStorage(const UITextEditStorage& other)
    : domain(other.domain), fontSize(other.fontSize)
{
    TextBuffer<char> otherBuf = other.buf;

    buf = TextBuffer<char>::create();
    buf.set_string(otherBuf.to_string().c_str());
}

UITextEditStorage::~UITextEditStorage()
{
    TextBuffer<char>::destroy(buf);
}

UITextEditStorage& UITextEditStorage::operator=(const UITextEditStorage& other)
{
    TextBuffer<char> otherBuf = other.buf;

    buf.set_string(otherBuf.to_string().c_str());
    domain = other.domain;
    fontSize = other.fontSize;

    return *this;
}

UITextEditStorage* UITextEditWidget::get_storage()
{
    return mObj->as.textEdit.storage;
}

void UITextEditWidget::set_text(View text)
{
    auto& self = mObj->as.textEdit;

    self.storage->buf.set_string(text);
}

void UITextEditWidget::set_domain(UITextEditDomain domain)
{
    auto& self = mObj->as.textEdit;

    if (self.storage->domain != domain)
    {
        self.storage->buf.clear();
        self.storage->domain = domain;
    }
}

void UITextEditWidget::set_on_change(UITextEditOnChange onChange)
{
    mObj->as.textEdit.onChange = onChange;
}

void UITextEditWidget::set_on_submit(UITextEditOnSubmit onSubmit)
{
    mObj->as.textEdit.onSubmit = onSubmit;
}

void UITextEditWidget::on_draw(UIWidget widget, ScreenRenderComponent renderer)
{
    UIWidgetObj* obj = widget.unwrap();
    UIContextObj& ctx = *obj->ctx();
    auto& self = obj->as.textEdit;
    const Rect& rect = obj->layout.rect;
    const UITheme& theme = ctx.theme;
    UITextEditStorage* storage = self.storage;
    std::string str;

    renderer.draw_rect(rect, theme.get_field_color());

    Color outlineColor = theme.get_selection_color();
    if (widget.is_focused())
        renderer.draw_rect_outline(rect, outlineColor, 1.0f);

    if (!storage->buf.empty())
    {
        float wrapWidth = rect.w;
        str = storage->buf.to_string();
        renderer.draw_text(ctx.fontAtlas, ctx.fontAtlasImage, storage->fontSize, rect.get_pos(), str.c_str(), theme.get_on_surface_color(), wrapWidth);
    }
}

} // namespace LD