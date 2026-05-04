#include <Ludens/DSA/ViewUtil.h>
#include <Ludens/UI/UIWidget.h>

#include "../UIContextObj.h"
#include "../UIWidgetMeta.h"
#include "../UIWidgetObj.h"
#include "UITextEditWidgetObj.h"

namespace LD {

static bool is_edit_key(KeyValue key)
{
    static const HashSet<KeyValue> sEditKeys = {
        KeyValue(KEY_CODE_ENTER),
        KeyValue(KEY_CODE_BACKSPACE),
        KeyValue(KEY_CODE_BACKSPACE, KEY_MOD_CONTROL_BIT),
        KeyValue(KEY_CODE_DELETE),
        KeyValue(KEY_CODE_ESCAPE),
        KeyValue(KEY_CODE_LEFT),
        KeyValue(KEY_CODE_RIGHT),
        KeyValue(KEY_CODE_A, KEY_MOD_CONTROL_BIT),
    };

    return sEditKeys.contains(key);
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

UILayoutInfo UITextEditWidgetObj::default_layout()
{
    return UILayoutInfo(UISize::fixed(100.0f), UISize::fixed(19.2f));
}

void UITextEditWidgetObj::startup(UIWidgetObj* obj)
{
    UIContextObj* ctx = obj->ctx();
    UITextEditWidgetObj& self = obj->U->textEdit;
    new (&self) UITextEditWidgetObj();
    self.connect(obj);

    UITextEditData& data = self.get_data();
    data.font = ctx->fontDefault;
    data.bgColor = ctx->theme.get_field_color();
    data.bgColorEdit = data.bgColor;

    obj->flags |= UI_WIDGET_FLAG_FOCUSABLE_BIT;
}

void UITextEditWidgetObj::cleanup(UIWidgetObj* obj)
{
    UITextEditWidgetObj& self = obj->U->textEdit;

    (&self)->~UITextEditWidgetObj();
}

bool UITextEditWidgetObj::on_event(UIWidgetObj* obj, const UIEvent& event)
{
    UITextEditWidgetObj& self = obj->U->textEdit;
    const UITextEditData& data = self.get_data();

    switch (event.type)
    {
    case UI_EVENT_KEY_DOWN:
        return self.on_key_down_event(event);
    case UI_EVENT_MOUSE_DOWN:
        self.on_mouse_down_event(event);
        return true;
    case UI_EVENT_MOUSE_DRAG:
        self.on_mouse_drag_event(event);
        return true;
    case UI_EVENT_FOCUS_ENTER:
        if (data.beginEditOnFocus)
            self.begin_edit();
        return true;
    case UI_EVENT_FOCUS_LEAVE:
        self.finish_edit();
        return true;
    default:
        break;
    }

    return false; // not handled
}

void UITextEditWidgetObj::on_draw(UIWidgetObj* obj, ScreenRenderComponent renderer)
{
    UIContextObj& ctx = *obj->ctx();
    auto& self = obj->U->textEdit;
    const Rect& rect = self.get_rect();
    UITextEditData& data = self.get_data();
    UITheme theme = ctx.theme;

    if (!data.font)
        data.font = ctx.fontDefault;

    UITextEditDrawInfo info{};
    info.renderer = renderer;
    info.atlas = data.font.font_atlas();
    info.image = data.font.image();
    info.textColor = theme.get_on_surface_color();
    info.outlineColor = theme.get_selection_color();
    info.fontSize = data.fontSize;
    info.str = data.mEditor.get_string();

    if (data.mIsEditing)
    {
        renderer.draw_rect(rect, data.bgColorEdit);
        self.draw_edit_state(info);
    }
    else
    {
        renderer.draw_rect(rect, data.bgColor);
        renderer.draw_text(info.atlas, info.image, data.fontSize, rect.get_pos(), info.str.c_str(), info.textColor, rect.w);
    }
}

CursorType UITextEditWidgetObj::cursor_hint(UIWidgetObj* obj)
{
    UITextEditWidgetObj& self = obj->U->textEdit;

    return self.get_data().is_editing() ? CURSOR_TYPE_IBEAM : CURSOR_TYPE_DEFAULT;
}

void UITextEditWidgetObj::begin_edit()
{
    UITextEditData& data = get_data();
    data.mIsEditing = true;
    data.mOriginal = data.mEditor.get_string();
}

void UITextEditWidgetObj::finish_edit()
{
    UITextEditData& data = get_data();
    if (!data.mIsEditing)
        return;

    data.mIsEditing = false;
    data.mOriginal = data.mEditor.get_string();

    base->ctx()->requestLooseFocus = base;
}

void UITextEditWidgetObj::cancel_edit()
{
    UITextEditData& data = get_data();
    data.mIsEditing = false;

    std::string canceled = data.mEditor.get_string();
    if (canceled != data.mOriginal)
    {
        data.mEditor.set_string(data.mOriginal);

        View view(data.mOriginal);
        if (data.onChange)
            data.onChange({base}, view, base->user);
    }

    base->ctx()->requestLooseFocus = base;
}

void UITextEditWidgetObj::on_mouse_down_event(const UIEvent& event)
{
    if (event.type != UI_EVENT_MOUSE_DOWN)
        return;

    UITextEditData& data = get_data();
    FontAtlas atlas = data.font.font_atlas();
    Rect rect = get_rect();

    std::string str = data.mEditor.get_string();
    int index = atlas.measure_cursor_index(view(str), data.fontSize, rect.w, event.mouse.position);
    if (index < 0)
        index = (int)str.size();

    data.mEditor.set_cursor((size_t)index);
}

void UITextEditWidgetObj::on_mouse_drag_event(const UIEvent& event)
{
    if (event.type != UI_EVENT_MOUSE_DRAG || event.drag.button != MOUSE_BUTTON_LEFT)
        return;

    UITextEditData& data = get_data();
    FontAtlas atlas = data.font.font_atlas();
    Rect rect = get_rect();

    std::string str = data.mEditor.get_string();
    Vec2 localPos = event.drag.position - rect.get_pos();
    int index = atlas.measure_cursor_index(view(str), data.fontSize, rect.w, localPos);

    if (event.drag.begin)
        data.mDragBeginPos = index < 0 ? str.size() : index;
    else
    {
        data.mDragPos = index < 0 ? str.size() : index;
        Range dragRange = Range::from_offsets(data.mDragPos, data.mDragBeginPos);
        data.mEditor.set_selection(dragRange);
    }
}

bool UITextEditWidgetObj::on_key_down_event(const UIEvent& event)
{
    UITextEditData& data = get_data();

    if (event.type != UI_EVENT_KEY_DOWN || !data.is_editing())
        return false;

    bool hasChanged = false;
    bool hasSubmitted = false;

    if (event.key.code == KEY_CODE_ESCAPE)
    {
        cancel_edit();
        return true;
    }

    switch (data.mDomain)
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

    std::string str = data.mEditor.get_string();
    View strView = view(str);

    if (hasChanged && data.onChange)
        data.onChange({base}, strView, base->user);

    if (hasSubmitted)
    {
        finish_edit();

        if (data.onSubmit)
            data.onSubmit({base}, strView, base->user);
    }

    return true;
}

void UITextEditWidgetObj::domain_string_on_key(const UIEvent& event, bool& hasChanged, bool& hasSubmitted)
{
    UITextEditData& data = get_data();
    LD_ASSERT(data.mDomain == UI_TEXT_EDIT_DOMAIN_STRING);

    TextEditLiteResult result = data.mEditor.key(KeyValue(event.key.code, event.key.mods));
    hasChanged = result == TEXT_EDIT_LITE_RESULT_CHANGED;
    hasSubmitted = result == TEXT_EDIT_LITE_RESULT_SUBMITTED;
}

void UITextEditWidgetObj::domain_uint_on_key(const UIEvent& event, bool& hasChanged, bool& hasSubmitted)
{
    UITextEditData& data = get_data();
    LD_ASSERT(data.mDomain == UI_TEXT_EDIT_DOMAIN_UINT);

    KeyValue key(event.key.code, event.key.mods);
    hasChanged = false;
    hasSubmitted = false;

    if (is_edit_key(key) || is_digit_key(key))
    {
        TextEditLiteResult result = data.mEditor.key(key);
        hasChanged = result == TEXT_EDIT_LITE_RESULT_CHANGED;
        hasSubmitted = result == TEXT_EDIT_LITE_RESULT_SUBMITTED;
    }
}

void UITextEditWidgetObj::domain_f32_on_key(const UIEvent& event, bool& hasChanged, bool& hasSubmitted)
{
    UITextEditData& data = get_data();
    LD_ASSERT(data.mDomain == UI_TEXT_EDIT_DOMAIN_F32);

    KeyValue key(event.key.code, event.key.mods);
    hasChanged = false;
    hasSubmitted = false;

    if (is_edit_key(key) || is_digit_key(key) || key == KeyCode(KEY_CODE_PERIOD))
    {
        TextEditLiteResult result = data.mEditor.key(key);
        hasChanged = result == TEXT_EDIT_LITE_RESULT_CHANGED;
        hasSubmitted = result == TEXT_EDIT_LITE_RESULT_SUBMITTED;
    }
}

void UITextEditWidgetObj::draw_edit_state(UITextEditDrawInfo& info)
{
    UITextEditData& data = get_data();
    Rect rect = get_rect();
    UITheme theme = base->ctx()->theme;
    Color outlineColor = theme.get_selection_color();

    info.renderer.draw_rect_outline(rect, outlineColor, 1.0f);

    float minWidth, maxWidth;
    Range selection = data.mEditor.get_selection();
    size_t cursor = data.mEditor.get_cursor();

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
        info.renderer.draw_text(info.atlas, info.image, data.fontSize, rect.get_pos(), info.str.c_str(), info.textColor, rect.w);
    }

    if (!selection) // draw cursor
    {
        info.atlas.measure_wrap_limit(View((const byte*)info.str.data(), cursor), info.fontSize, minWidth, maxWidth);
        float cursorX = maxWidth;
        const float beamWidth = 2.0f; // TODO:
        info.renderer.draw_rect(Rect(rect.x + cursorX, rect.y, beamWidth, rect.h), info.textColor);
    }
}

UITextEditData::UITextEditData()
{
    mEditor = TextEditLite::create();
}

UITextEditData::UITextEditData(const UITextEditData& other)
    : mDomain(other.mDomain), fontSize(other.fontSize)
{
    TextEditLite otherEditor = other.mEditor;

    mEditor = TextEditLite::create();
    mEditor.set_string(otherEditor.get_string());
}

UITextEditData::~UITextEditData()
{
    TextEditLite::destroy(mEditor);
}

UITextEditData& UITextEditData::operator=(const UITextEditData& other)
{
    TextEditLite otherEditor = other.mEditor;

    mEditor.set_string(otherEditor.get_string());
    mDomain = other.mDomain;
    fontSize = other.fontSize;

    return *this;
}

void UITextEditData::set_text(const std::string& str)
{
    mOriginal = str;
    mEditor.set_string(str);
}

void UITextEditData::set_domain(UITextEditDomain mDomain)
{
    if (this->mDomain != mDomain)
    {
        mEditor.clear();
        mOriginal.clear();
        this->mDomain = mDomain;
    }
}

bool UITextEditWidget::try_begin_edit()
{
    UITextEditWidgetObj& self = mObj->U->textEdit;

    if (is_focused())
    {
        self.begin_edit();
        return true;
    }

    return false;
}

bool UITextEditWidget::is_editing()
{
    UITextEditWidgetObj& self = mObj->U->textEdit;

    return self.get_data().is_editing();
}

void UITextEditWidget::set_on_change(UITextEditOnChange onChange)
{
    UITextEditWidgetObj& self = mObj->U->textEdit;

    self.get_data().onChange = onChange;
}

void UITextEditWidget::set_on_submit(UITextEditOnSubmit onSubmit)
{
    UITextEditWidgetObj& self = mObj->U->textEdit;

    self.get_data().onSubmit = onSubmit;
}

} // namespace LD