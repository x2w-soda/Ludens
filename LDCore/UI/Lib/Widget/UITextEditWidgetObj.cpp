#include <Ludens/UI/UIWidget.h>

#include "../UIContextObj.h"
#include "../UIWidgetMeta.h"
#include "../UIWidgetObj.h"
#include "UITextEditWidgetObj.h"

namespace LD {

static bool is_edit_key(KeyValue key)
{
    return key == KeyValue(KEY_CODE_ENTER) || key == KeyCode(KEY_CODE_BACKSPACE) || key == KeyCode(KEY_CODE_DELETE) || key == KeyCode(KEY_CODE_ESCAPE) || key == KeyCode(KEY_CODE_LEFT) || key == KeyCode(KEY_CODE_RIGHT);
}

static bool is_digit_key(KeyValue key)
{
    return (KEY_CODE_0 <= key.code() && key.code() <= KEY_CODE_9 && key.mods() == 0) ||
           (KEY_CODE_KEYPAD_0 <= key.code() && key.code() <= KEY_CODE_KEYPAD_9);
}

void UITextEditWidgetObj::startup(UIWidgetObj* obj, void* storage)
{
    UITextEditWidgetObj& self = obj->as.textEdit;
    new (&self) UITextEditWidgetObj();
    self.base = obj;
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
        self.begin_edit();
        return true;
    case UI_EVENT_FOCUS_LEAVE:
        self.finish_edit();
        return true;
    case UI_EVENT_MOUSE_DOWN:
        self.on_mouse_down_event(event);
        return true;
    case UI_EVENT_KEY_DOWN:
        self.on_key_down_event(event);
        return true;
    default:
        break;
    }

    return false; // not handled
}

void UITextEditWidgetObj::begin_edit()
{
    isEditing = true;
    storage->original = storage->editor.get_string();
}

void UITextEditWidgetObj::finish_edit()
{
    isEditing = false;
    storage->original = storage->editor.get_string();
}

void UITextEditWidgetObj::cancel_edit()
{
    isEditing = false;

    std::string canceled = storage->editor.get_string();
    if (canceled != storage->original)
    {
        storage->editor.set_string(storage->original);

        View view(storage->original.data(), storage->original.size());
        if (onChange)
            onChange({base}, view, base->user);
    }
}

void UITextEditWidgetObj::on_mouse_down_event(const UIEvent& event)
{
    if (event.type != UI_EVENT_MOUSE_DOWN)
        return;

    FontAtlas atlas = storage->font.font_atlas();
    Rect rect = base->layout.rect;

    std::string str = storage->editor.get_string();
    int index = atlas.measure_text_index(View(str.data(), str.size()), storage->fontSize, rect.w, event.mouse.position);
    if (index >= 0)
        storage->editor.set_cursor((size_t)index);
}

void UITextEditWidgetObj::on_key_down_event(const UIEvent& event)
{
    if (event.type != UI_EVENT_KEY_DOWN)
        return;

    bool hasChanged = false;
    bool hasSubmitted = false;

    if (event.key.code == KEY_CODE_ESCAPE)
    {
        cancel_edit();
        return;
    }

    switch (storage->domain)
    {
    case UI_TEXT_EDIT_DOMAIN_STRING:
        domain_string_on_key(event, hasChanged, hasSubmitted);
        break;
    case UI_TEXT_EDIT_DOMAIN_UINT:
        domain_uint_on_key(event, hasChanged, hasSubmitted);
        break;
    case UI_TEXT_EDIT_DOMAIN_F32:
        domain_f32_on_key(event, hasChanged, hasSubmitted);
        break;
    default:
        break;
    }

    std::string str = storage->editor.get_string();
    View strView(str.data(), str.size());

    if (hasChanged && onChange)
        onChange({base}, strView, base->user);

    if (hasSubmitted)
    {
        finish_edit();

        if (onSubmit)
            onSubmit({base}, strView, base->user);
    }
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

    KeyValue key(event.key.code, event.key.mods);
    hasChanged = false;
    hasSubmitted = false;

    if (is_edit_key(key) || is_digit_key(key))
    {
        TextEditLiteResult result = storage->editor.key(key);
        hasChanged = result == TEXT_EDIT_LITE_RESULT_CHANGED;
        hasSubmitted = result == TEXT_EDIT_LITE_RESULT_SUBMITTED;
    }
}

void UITextEditWidgetObj::domain_f32_on_key(const UIEvent& event, bool& hasChanged, bool& hasSubmitted)
{
    LD_ASSERT(storage->domain == UI_TEXT_EDIT_DOMAIN_F32);

    KeyValue key(event.key.code, event.key.mods);
    hasChanged = false;
    hasSubmitted = false;

    if (is_edit_key(key) || is_digit_key(key) || key == KeyCode(KEY_CODE_PERIOD))
    {
        TextEditLiteResult result = storage->editor.key(key);
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

void UITextEditStorage::set_text(const std::string& str)
{
    original = str;
    editor.set_string(str);
}

void UITextEditStorage::set_domain(UITextEditDomain domain)
{
    if (this->domain != domain)
    {
        editor.clear();
        original.clear();
        this->domain = domain;
    }
}

UITextEditStorage* UITextEditWidget::get_storage()
{
    return mObj->as.textEdit.storage;
}

bool UITextEditWidget::is_editing()
{
    return mObj->as.textEdit.isEditing;
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
    str = storage->editor.get_string();

    if (self.isEditing)
    {
        renderer.draw_rect_outline(rect, outlineColor, 1.0f);

        float minWidth, maxWidth;
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
        renderer.draw_text(atlas, image, storage->fontSize, rect.get_pos(), str.c_str(), textColor, wrapWidth);
    }
}

} // namespace LD