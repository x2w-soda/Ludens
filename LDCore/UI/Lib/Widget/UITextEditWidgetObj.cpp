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

struct UITextEditDrawInfo
{
    ScreenRenderComponent renderer;
    FontAtlas atlas;
    RImage image;
    Color textColor;
    Color outlineColor;
    float fontSize;
    std::string str;
};

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
}

void UITextEditWidgetObj::cleanup(UIWidgetObj* base)
{
    UITextEditWidgetObj* obj = &base->as.textEdit;

    obj->~UITextEditWidgetObj();
}

bool UITextEditWidgetObj::on_event(UIWidgetObj* obj, const UIEvent& event)
{
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

void UITextEditWidgetObj::on_draw(UIWidgetObj* obj, ScreenRenderComponent renderer)
{
    UIContextObj& ctx = *obj->ctx();
    auto& self = obj->as.textEdit;
    const Rect& rect = obj->layout.rect;
    UITheme theme = ctx.theme;
    UITextEditStorage* storage = self.storage;

    renderer.draw_rect(rect, theme.get_field_color());

    if (!storage->font)
        storage->font = ctx.fontDefault;

    UITextEditDrawInfo info{};
    info.renderer = renderer;
    info.atlas = storage->font.font_atlas();
    info.image = storage->font.image();
    info.textColor = theme.get_on_surface_color();
    info.outlineColor = theme.get_selection_color();
    info.fontSize = storage->fontSize;
    info.str = storage->editor.get_string();

    if (self.isEditing)
    {
        self.draw_edit_state(info);
    }
    else
    {
        renderer.draw_text(info.atlas, info.image, storage->fontSize, rect.get_pos(), info.str.c_str(), info.textColor, rect.w);
    }
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

    base->ctx()->requestLooseFocus = base;
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

    base->ctx()->requestLooseFocus = base;
}

void UITextEditWidgetObj::on_mouse_down_event(const UIEvent& event)
{
    if (event.type != UI_EVENT_MOUSE_DOWN)
        return;

    FontAtlas atlas = storage->font.font_atlas();
    Rect rect = base->layout.rect;

    std::string str = storage->editor.get_string();
    int index = atlas.measure_cursor_index(View(str.data(), str.size()), storage->fontSize, rect.w, event.mouse.position);
    if (index < 0)
        index = (int)str.size();

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

void UITextEditWidgetObj::draw_edit_state(UITextEditDrawInfo& info)
{
    Rect rect = base->layout.rect;
    UITheme theme = base->ctx()->theme;
    Color outlineColor = theme.get_selection_color();

    info.renderer.draw_rect_outline(rect, outlineColor, 1.0f);

    float minWidth, maxWidth;
    Range selection = storage->editor.get_selection();
    size_t cursor = storage->editor.get_cursor();

    if (selection) // draw selection area
    {
        int queries[2] = {selection.offset, selection.offset + selection.size};
        Vec2 positions[2];
        info.atlas.measure_baseline_positions(View(info.str), info.fontSize, rect.w, 2, queries, positions);

        float startX = positions[0].x;
        float endX = positions[1].x;
        info.renderer.draw_rect(Rect(rect.x + startX, rect.y, endX - startX, rect.h), 0x404040FF);
    }

    if (!info.str.empty())
    {
        info.renderer.draw_text(info.atlas, info.image, storage->fontSize, rect.get_pos(), info.str.c_str(), info.textColor, rect.w);
    }

    if (!selection) // draw cursor
    {
        info.atlas.measure_wrap_limit(View(info.str.data(), cursor), info.fontSize, minWidth, maxWidth);
        float cursorX = maxWidth;
        const float beamWidth = 2.0f; // TODO:
        info.renderer.draw_rect(Rect(rect.x + cursorX, rect.y, beamWidth, rect.h), info.textColor);
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

void UITextEditWidget::set_storage(UITextEditStorage* storage)
{
    mObj->as.textEdit.storage = storage;
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

} // namespace LD