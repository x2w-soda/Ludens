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
        self.storage->font = obj->ctx()->fontDefault;
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

    switch (event.type)
    {
    case UI_EVENT_FOCUS_ENTER:
        self.isEditing = true;
        return true;
    case UI_EVENT_FOCUS_LEAVE:
        self.isEditing = false;
        return true;
    case UI_EVENT_MOUSE_DOWN:
        return true;
    case UI_EVENT_KEY_DOWN:
        break;
    default:
        return false; // not handled
    }

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

    std::string str = self.storage->editor.get_string();
    View strView(str.data(), str.size());

    if (hasChanged && self.onChange)
        self.onChange((UITextEditWidget)widget, strView, obj->user);

    if (hasSubmitted)
    {
        self.storage->text = str;

        if (self.onSubmit)
            self.onSubmit((UITextEditWidget)widget, strView, obj->user);
    }

    return true;
}

void UITextEditWidgetObj::domain_string_on_key(const UIEvent& event, bool& hasChanged, bool& hasSubmitted)
{
    LD_ASSERT(storage->domain == UI_TEXT_EDIT_DOMAIN_STRING);

    TextEditLiteResult result = storage->editor.key(KeyValue(event.key.code, event.key.mods));
    hasChanged = result == TEXT_EDIT_LITE_RESULT_CHANGED;
    hasSubmitted = result == TEXT_EDIT_LITE_RESULT_SUBMITTED;
}

void UITextEditWidgetObj::domain_uint_on_key(const UIEvent& event, bool& hasChanged, bool& hasSubmitted)
{
    LD_ASSERT(storage->domain == UI_TEXT_EDIT_DOMAIN_UINT);

    const KeyCode code = event.key.code;
    const KeyMods mods = event.key.mods;

    hasChanged = false;
    hasSubmitted = false;

    // TODO: we could still input @!#$
    if ((KEY_CODE_0 <= code && code <= KEY_CODE_9) || code == KEY_CODE_BACKSPACE || code == KEY_CODE_ENTER)
    {
        TextEditLiteResult result = storage->editor.key(KeyValue(event.key.code, event.key.mods));
        hasChanged = result == TEXT_EDIT_LITE_RESULT_CHANGED;
        hasSubmitted = result == TEXT_EDIT_LITE_RESULT_SUBMITTED;
    }
}

UITextEditStorage::UITextEditStorage()
{
    editor = TextEditLite::create();
}

UITextEditStorage::UITextEditStorage(const UITextEditStorage& other)
    : domain(other.domain), fontSize(other.fontSize)
{
    TextEditLite otherEditor = other.editor;

    editor = TextEditLite::create();
    editor.set_string(otherEditor.get_string());
}

UITextEditStorage::~UITextEditStorage()
{
    TextEditLite::destroy(editor);
}

UITextEditStorage& UITextEditStorage::operator=(const UITextEditStorage& other)
{
    TextEditLite otherEditor = other.editor;

    editor.set_string(otherEditor.get_string());
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

    self.storage->editor.set_string(text);
}

void UITextEditWidget::set_domain(UITextEditDomain domain)
{
    auto& self = mObj->as.textEdit;

    if (self.storage->domain != domain)
    {
        self.storage->editor.clear();
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

    Color textColor = theme.get_on_surface_color();
    Color outlineColor = theme.get_selection_color();
    float wrapWidth = rect.w;

    if (!storage->font)
        storage->font = ctx.fontDefault;

    FontAtlas atlas = storage->font.font_atlas();
    RImage image = storage->font.image();

    if (self.isEditing)
    {
        renderer.draw_rect_outline(rect, outlineColor, 1.0f);

        float minWidth, maxWidth;
        str = storage->editor.get_string();
        size_t cursor = storage->editor.get_cursor();
        atlas.measure_wrap_limit(View(str.data(), cursor), storage->fontSize, minWidth, maxWidth);
        float cursorX = maxWidth;

        if (!str.empty())
        {
            renderer.draw_text(atlas, image, storage->fontSize, rect.get_pos(), str.c_str(), textColor, wrapWidth);
        }

        const float beamWidth = 2.0f; // TODO:
        renderer.draw_rect(Rect(rect.x + cursorX, rect.y, beamWidth, rect.h), textColor);
    }
    else
    {
        renderer.draw_text(atlas, image, storage->fontSize, rect.get_pos(), self.storage->text.c_str(), textColor, wrapWidth);
    }
}

} // namespace LD