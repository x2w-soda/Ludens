#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Types.h>
#include <Ludens/UI/UIWidget.h>
#include <Ludens/UI/UIWindow.h>
#include <Ludens/WindowRegistry/Input.h>
#include <algorithm>

#include "UIObj.h"

namespace LD {

// clang-format off
struct
{
    UIWidgetType type;
    const char* typeName;
    size_t objSize;
    void (*cleanup)(UIWidgetObj* obj);
} sWidgetTable[] = {
    { UI_WIDGET_WINDOW,    "UIWindow",   sizeof(UIWindowObj),         nullptr },
    { UI_WIDGET_SCROLL,    "UIScroll",   sizeof(UIScrollWidgetObj),   nullptr },
    { UI_WIDGET_BUTTON,    "UIButton",   sizeof(UIButtonWidgetObj),   &UIButtonWidgetObj::cleanup  },
    { UI_WIDGET_SLIDER,    "UISlider",   sizeof(UISliderWidgetObj),   nullptr },
    { UI_WIDGET_TOGGLE,    "UIToggle",   sizeof(UIToggleWidgetObj),   nullptr },
    { UI_WIDGET_PANEL,     "UIPanel",    sizeof(UIPanelWidgetObj),    nullptr },
    { UI_WIDGET_IMAGE,     "UIImage",    sizeof(UIImageWidgetObj),    nullptr },
    { UI_WIDGET_TEXT,      "UIText",     sizeof(UITextWidgetObj),     &UITextWidgetObj::cleanup },
    { UI_WIDGET_TEXT_EDIT, "UITextEdit", sizeof(UITextEditWidgetObj), &UITextEditWidgetObj::cleanup },
};
// clang-format on

static_assert(sizeof(sWidgetTable) / sizeof(*sWidgetTable) == UI_WIDGET_TYPE_COUNT);
static_assert(IsTrivial<UIScrollWidgetObj>);
static_assert(IsTrivial<UITextWidgetObj>);
static_assert(IsTrivial<UIPanelWidgetObj>);
static_assert(IsTrivial<UIImageWidgetObj>);
static_assert(IsTrivial<UIToggleWidgetObj>);
static_assert(IsTrivial<UISliderWidgetObj>);
static_assert(IsTrivial<UIButtonWidgetObj>);

UIWidgetObj::UIWidgetObj(UIWidgetType type, const UILayoutInfo& layoutI, UIWidgetObj* parent, UIWindowObj* window, void* user)
    : window(window), type(type), parent(parent), user(user)
{
    LD_ASSERT(window);

    layout.info = layoutI;
    layout.rect = {};
    node = UINode(this);

    switch (type)
    {
    case UI_WIDGET_TEXT_EDIT:
        new (&as.textEdit) UITextEditWidgetObj();
        break;
    }
}

UIWidgetObj::~UIWidgetObj()
{
    switch (type)
    {
    case UI_WIDGET_TEXT_EDIT:
        (&as.textEdit)->~UITextEditWidgetObj();
        break;
    }
}

UIContextObj* UIWidgetObj::ctx() const
{
    // crazy pointer chasing
    return window->space->layer->ctx;
}

void UIWidgetObj::draw(ScreenRenderComponent renderer)
{
    if (flags & UI_WIDGET_FLAG_HIDDEN_BIT)
        return;

    if (cb.onDraw)
    {
        cb.onDraw(UIWidget(this), renderer);
        return;
    }

    switch (type)
    {
    case UI_WIDGET_WINDOW:
        UIWindowObj::on_draw(UIWidget(this), renderer);
        break;
    case UI_WIDGET_PANEL:
        UIPanelWidget::on_draw(UIWidget(this), renderer);
        break;
    case UI_WIDGET_BUTTON:
        UIButtonWidget::on_draw(UIWidget(this), renderer);
        break;
    case UI_WIDGET_SLIDER:
        UISliderWidget::on_draw(UIWidget(this), renderer);
        break;
    case UI_WIDGET_TOGGLE:
        UIToggleWidget::on_draw(UIWidget(this), renderer);
        break;
    case UI_WIDGET_IMAGE:
        UIImageWidget::on_draw(UIWidget(this), renderer);
        break;
    case UI_WIDGET_TEXT:
        UITextWidget::on_draw(UIWidget(this), renderer);
        break;
    case UI_WIDGET_TEXT_EDIT:
        UITextEditWidget::on_draw(UIWidget(this), renderer);
        break;
    default:
        LD_UNREACHABLE;
    }
}

bool UIWidget::is_hovered()
{
    UIContextObj* ctx = mObj->ctx();
    UIWidgetObj* hovered = ctx->hoverWidgetLeaf;

    while (hovered)
    {
        if (hovered == mObj)
            return true;

        hovered = hovered->parent;
    }

    return false;
}

bool UIWidget::is_focused()
{
    UIContextObj* ctx = mObj->ctx();
    return ctx->focusWidget == mObj;
}

bool UIWidget::is_pressed()
{
    UIContextObj* ctx = mObj->ctx();
    return ctx->pressWidget == mObj;
}

bool UIWidget::is_dragged()
{
    UIContextObj* ctx = mObj->ctx();
    return ctx->dragWidget == mObj;
}

UINode& UIWidget::node()
{
    return mObj->node;
}

void UIWidget::set_visible(bool isVisible)
{
    if (isVisible)
        mObj->flags &= ~UI_WIDGET_FLAG_HIDDEN_BIT;
    else
        mObj->flags |= UI_WIDGET_FLAG_HIDDEN_BIT;
}

bool UIWidget::is_visible()
{
    return !static_cast<bool>(mObj->flags & UI_WIDGET_FLAG_HIDDEN_BIT);
}

void UIWidget::block_input()
{
    mObj->flags |= UI_WIDGET_FLAG_BLOCK_EVENT_BIT;
}

void UIWidget::unblock_input()
{
    mObj->flags &= ~UI_WIDGET_FLAG_BLOCK_EVENT_BIT;
}

UIWidgetType UIWidget::get_type()
{
    return mObj->type;
}

Rect UIWidget::get_rect()
{
    return mObj->layout.rect;
}

Vec2 UIWidget::get_pos()
{
    return mObj->layout.rect.get_pos();
}

Vec2 UIWidget::get_size()
{
    return mObj->layout.rect.get_size();
}

UITheme UIWidget::get_theme()
{
    return mObj->theme;
}

bool UIWidget::get_mouse_pos(Vec2& pos)
{
    UIContextObj* ctx = mObj->ctx();

    const Rect& widgetRect = mObj->layout.rect;

    if (widgetRect.contains(ctx->cursorPos))
    {
        pos = ctx->cursorPos - widgetRect.get_pos();
        return true;
    }

    return false;
}

void* UIWidget::get_user()
{
    return mObj->user;
}

void UIWidget::set_user(void* user)
{
    mObj->user = user;
}

void UIWidget::get_name(std::string& name)
{
    name = mObj->name;
}

void UIWidget::set_name(const std::string& name)
{
    mObj->name = name;
}

void UIWidget::set_consume_mouse_event(bool consumes)
{
    if (consumes)
        mObj->flags |= UI_WIDGET_FLAG_CONSUME_MOUSE_EVENT_BIT;
    else
        mObj->flags &= ~UI_WIDGET_FLAG_CONSUME_MOUSE_EVENT_BIT;
}

void UIWidget::set_consume_key_event(bool consumes)
{
    if (consumes)
        mObj->flags |= UI_WIDGET_FLAG_CONSUME_KEY_EVENT_BIT;
    else
        mObj->flags &= ~UI_WIDGET_FLAG_CONSUME_KEY_EVENT_BIT;
}

void UIWidget::set_consume_scroll_event(bool consumes)
{
    if (consumes)
        mObj->flags |= UI_WIDGET_FLAG_CONSUME_SCROLL_EVENT_BIT;
    else
        mObj->flags &= ~UI_WIDGET_FLAG_CONSUME_SCROLL_EVENT_BIT;
}

UIWidget UIWidget::get_child_by_name(const std::string& childName)
{
    return UIWidget(mObj->get_child_by_name(childName));
}

void UIWidget::get_layout(UILayoutInfo& layout)
{
    layout = mObj->layout.info;
}

void UIWidget::set_layout(const UILayoutInfo& layout)
{
    mObj->layout.info = layout;
}

void UIWidget::set_layout_size(const UISize& sizeX, const UISize& sizeY)
{
    mObj->layout.info.sizeX = sizeX;
    mObj->layout.info.sizeY = sizeY;
}

void UIWidget::set_layout_child_padding(const UIPadding& padding)
{
    mObj->layout.info.childPadding = padding;
}

void UIWidget::set_layout_child_gap(float gap)
{
    mObj->layout.info.childGap = gap;
}

void UIWidget::set_layout_child_axis(UIAxis axis)
{
    mObj->layout.info.childAxis = axis;
}

void UIWidget::set_layout_child_align_x(UIAlign childAlignX)
{
    mObj->layout.info.childAlignX = childAlignX;
}

void UIWidget::set_layout_child_align_y(UIAlign childAlignY)
{
    mObj->layout.info.childAlignY = childAlignY;
}

void UIWidget::set_on_update(void (*onUpdate)(UIWidget widget, float delta))
{
    mObj->cb.onUpdate = onUpdate;
}

void UIWidget::set_on_draw(void (*onDraw)(UIWidget widget, ScreenRenderComponent renderer))
{
    mObj->cb.onDraw = onDraw;
}

void UIWidget::set_on_event(bool (*onEvent)(UIWidget widget, const UIEvent& event))
{
    mObj->cb.onEvent = onEvent;
}

bool UIWidget::has_on_event()
{
    return mObj->cb.onEvent != nullptr;
}

UIContextObj* UINode::get_context()
{
    return mObj->ctx();
}

void UINode::get_children(std::vector<UIWidget>& widgets)
{
    widgets.clear();

    for (UIWidgetObj* child = mObj->child; child; child = child->next)
        widgets.push_back(UIWidget(child));
}

void UINode::remove()
{
    UIContextObj* ctx = mObj->ctx();

    ctx->free_widget(mObj);
    mObj = nullptr;
}

//
// UIScrollWidget
//

UIScrollWidget UINode::add_scroll(const UILayoutInfo& layoutI, const UIScrollWidgetInfo& widgetI, void* user)
{
    UIWidgetObj* obj = mObj->ctx()->alloc_widget(UI_WIDGET_SCROLL, layoutI, mObj, user);
    obj->as.scroll.base = obj;
    obj->as.scroll.bgColor = widgetI.bgColor;
    obj->as.scroll.offsetXDst = 0.0f;
    obj->as.scroll.offsetXSpeed = 0.0f;
    obj->as.scroll.offsetYDst = 0.0f;
    obj->as.scroll.offsetYSpeed = 0.0f;
    obj->cb.onDraw = &UIScrollWidget::on_draw;
    obj->cb.onUpdate = &UIScrollWidget::on_update;
    obj->cb.onEvent = &UIScrollWidgetObj::on_event;
    obj->flags |= UI_WIDGET_FLAG_DRAW_WITH_SCISSOR_BIT;

    return {obj};
}

void UIScrollWidgetObj::cleanup(UIWidgetObj* base)
{
    // TODO:
    (void)base;
}

bool UIScrollWidgetObj::on_event(UIWidget widget, const UIEvent& event)
{
    UIWidgetObj* base = (UIWidgetObj*)widget;
    UIScrollWidgetObj& self = base->as.scroll;

    const float sensitivity = 20.0f;
    const float animDuration = 0.14f;

    if (event.type != UI_EVENT_SCROLL)
        return false;

    const Vec2& offset = event.scroll.offset;

    if (offset.x != 0.0f)
    {
        self.offsetXDst += offset.x * sensitivity;
        self.offsetXSpeed = (self.offsetXDst - base->scrollOffset.x) / animDuration;

        if (self.offsetXDst > 0.0f)
            self.offsetXDst = 0.0f;
    }

    if (offset.y != 0.0f)
    {
        self.offsetYDst += offset.y * sensitivity;
        self.offsetYSpeed = (self.offsetYDst - base->scrollOffset.y) / animDuration;

        if (self.offsetYDst > 0.0f)
            self.offsetYDst = 0.0f;
    }

    return true;
}

void UIScrollWidget::set_scroll_offset_x(float offset)
{
    mObj->scrollOffset.x = offset;
    mObj->as.scroll.offsetXDst = offset;
    mObj->as.scroll.offsetXSpeed = 0.0f;
}

void UIScrollWidget::set_scroll_offset_y(float offset)
{
    mObj->scrollOffset.y = offset;
    mObj->as.scroll.offsetYDst = offset;
    mObj->as.scroll.offsetYSpeed = 0.0f;
}

void UIScrollWidget::set_scroll_bg_color(Color color)
{
    mObj->as.scroll.bgColor = color;
}

void UIScrollWidget::on_update(UIWidget widget, float delta)
{
    UIWidgetObj* base = widget.unwrap();
    UIScrollWidgetObj& self = base->as.scroll;

    if (self.offsetXSpeed != 0.0f)
    {
        base->scrollOffset.x += self.offsetXSpeed * delta;

        if ((self.offsetXSpeed > 0.0f && base->scrollOffset.x > self.offsetXDst) ||
            (self.offsetXSpeed < 0.0f && base->scrollOffset.x < self.offsetXDst))
        {
            base->scrollOffset.x = self.offsetXDst;
            self.offsetXSpeed = 0.0f;
        }
    }

    if (self.offsetYSpeed != 0.0f)
    {
        base->scrollOffset.y += self.offsetYSpeed * delta;

        if ((self.offsetYSpeed > 0.0f && base->scrollOffset.y > self.offsetYDst) ||
            (self.offsetYSpeed < 0.0f && base->scrollOffset.y < self.offsetYDst))
        {
            base->scrollOffset.y = self.offsetYDst;
            self.offsetYSpeed = 0.0f;
        }
    }
}

void UIScrollWidget::on_draw(UIWidget widget, ScreenRenderComponent renderer)
{
    UIWidgetObj* obj = widget;
    UIScrollWidgetObj& self = obj->as.scroll;
    Rect rect = widget.get_rect();

    renderer.draw_rect(rect, self.bgColor);
}

//
// UIImageWidget
//

UIImageWidget UINode::add_image(const UILayoutInfo& layoutI, const UIImageWidgetInfo& widgetI, void* user)
{
    UIWidgetObj* obj = mObj->ctx()->alloc_widget(UI_WIDGET_IMAGE, layoutI, mObj, user);
    obj->as.image.base = obj;
    obj->as.image.imageHandle = widgetI.image;
    obj->as.image.imageRect.w = 0;
    obj->as.image.tint = 0xFFFFFFFF;

    if (widgetI.rect)
        obj->as.image.imageRect = *widgetI.rect;

    return {obj};
}

void UIImageWidget::on_draw(UIWidget widget, ScreenRenderComponent renderer)
{
    UIImageWidgetObj& self = static_cast<UIWidgetObj*>(widget)->as.image;
    Rect rect = widget.get_rect();
    float imageW = (float)self.imageHandle.width();
    float imageH = (float)self.imageHandle.height();

    if (self.imageRect.w <= 0.0f)
    {
        renderer.draw_image(rect, self.tint, self.imageHandle, Rect(0.0f, 0.0f, 1.0f, 1.0f), false);
    }
    else
    {
        Rect uv = self.imageRect;
        uv.x /= imageW;
        uv.y /= imageH;
        uv.w /= imageW;
        uv.h /= imageH;
        renderer.draw_image(rect, self.tint, self.imageHandle, uv, false);
    }
}

RImage UIImageWidget::get_image()
{
    return mObj->as.image.imageHandle;
}

void UIImageWidget::set_image_rect(const Rect& rect)
{
    mObj->as.image.imageRect = rect;
}

Rect UIImageWidget::get_image_rect()
{
    return mObj->as.image.imageRect;
}

void UIImageWidget::set_image_tint(Color color)
{
    mObj->as.image.tint = color;
}

//
// UIButtonWidget
//

UIButtonWidget UINode::add_button(const UILayoutInfo& layoutI, const UIButtonWidgetInfo& widgetI, void* user)
{
    UIWidgetObj* obj = mObj->ctx()->alloc_widget(UI_WIDGET_BUTTON, layoutI, mObj, user);
    obj->cb.onEvent = UIButtonWidgetObj::on_event;
    obj->as.button.base = obj;
    obj->as.button.text = widgetI.text ? heap_strdup(widgetI.text, MEMORY_USAGE_UI) : nullptr;
    obj->as.button.onClick = widgetI.onClick;
    obj->as.button.textColor = widgetI.textColor;
    obj->as.button.transparentBG = widgetI.transparentBG;

    UIButtonWidget handle{obj};
    return handle;
};

//
// UISliderWidget
//

UISliderWidget UINode::add_slider(const UILayoutInfo& layoutI, const UISliderWidgetInfo& widgetI, void* user)
{
    UIWidgetObj* obj = mObj->ctx()->alloc_widget(UI_WIDGET_SLIDER, layoutI, mObj, user);
    obj->cb.onEvent = &UISliderWidgetObj::on_event;
    obj->as.slider.base = obj;
    obj->as.slider.min = widgetI.min;
    obj->as.slider.max = widgetI.max;
    obj->as.slider.value = widgetI.min;
    obj->as.slider.ratio = 0.0f;

    return {obj};
}

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

//
// UIToggleWidget
//

UIToggleWidget UINode::add_toggle(const UILayoutInfo& layoutI, const UIToggleWidgetInfo& widgetI, void* user)
{
    UIWidgetObj* obj = mObj->ctx()->alloc_widget(UI_WIDGET_TOGGLE, layoutI, mObj, user);
    obj->cb.onEvent = &UIToggleWidgetObj::on_event;
    obj->cb.onUpdate = &UIToggleWidgetObj::on_update;
    obj->as.toggle.base = obj;
    obj->as.toggle.state = widgetI.state;
    obj->as.toggle.user_on_toggle = widgetI.on_toggle;
    obj->as.toggle.anim.reset(1.0f);

    return {obj};
}

//
// UITextWidget
//

UITextWidget UINode::add_text(const UILayoutInfo& layoutI, const UITextWidgetInfo& widgetI, void* user)
{
    UILayoutInfo textLayoutI = layoutI;
    if (!(layoutI.sizeX.type == UI_SIZE_FIXED && layoutI.sizeX.extent > 0.0f))
    {
        textLayoutI.sizeX = UISize::wrap();
        textLayoutI.sizeY = UISize::fit();
    }

    UIContextObj* ctx = mObj->ctx();
    UIWidgetObj* obj = ctx->alloc_widget(UI_WIDGET_TEXT, textLayoutI, mObj, user);
    obj->as.text.fontSize = widgetI.fontSize;
    obj->as.text.value = widgetI.cstr ? heap_strdup(widgetI.cstr, MEMORY_USAGE_UI) : nullptr;
    obj->as.text.fontAtlas = ctx->fontAtlas;
    obj->as.text.fontImage = ctx->fontAtlasImage;
    obj->as.text.bgColor = 0;
    obj->as.text.fgColor = ctx->theme.get_on_surface_color();

    if (widgetI.bgColor)
        obj->as.text.bgColor = *widgetI.bgColor;

    return {obj};
}

void UITextWidgetObj::cleanup(UIWidgetObj* base)
{
    UITextWidgetObj& self = base->as.text;

    if (self.value)
    {
        heap_free((void*)self.value);
        self.value = nullptr;
    }

    self.~UITextWidgetObj();
}

void UITextWidget::on_draw(UIWidget widget, ScreenRenderComponent renderer)
{
    UIWidgetObj* obj = (UIWidgetObj*)widget;
    UIContextObj& ctx = *obj->ctx();
    const UITheme& theme = obj->theme;
    UITextWidgetObj& self = obj->as.text;
    Rect rect = widget.get_rect();
    float wrapWidth = rect.w;

    if (self.value && rect.h == 0) // likely a layout bug in UI text wrapping
        LD_DEBUG_BREAK;

    if (self.bgColor.get_alpha() > 0.0f)
        renderer.draw_rect(rect, self.bgColor);

    renderer.draw_text(self.fontAtlas, self.fontImage, self.fontSize, rect.get_pos(), self.value, self.fgColor, wrapWidth);
}

void UITextWidget::set_text(const char* cstr)
{
    if (mObj->as.text.value)
        heap_free((void*)mObj->as.text.value);

    mObj->as.text.value = cstr ? heap_strdup(cstr, MEMORY_USAGE_UI) : nullptr;
}

const char* UITextWidget::get_text()
{
    return mObj->as.text.value;
}

void UITextWidget::set_text_style(Color color, FontAtlas fontAtlas, RImage fontImage)
{
    mObj->as.text.fgColor = color;

    if (fontAtlas && fontImage)
    {
        mObj->as.text.fontAtlas = fontAtlas;
        mObj->as.text.fontImage = fontImage;
    }
}

float* UITextWidget::font_size()
{
    return &mObj->as.text.fontSize;
}

//
// UITextEditWidget
//

UITextEditWidget UINode::add_text_edit(const UILayoutInfo& layoutI, const UITextEditWidgetInfo& widgetI, void* user)
{
    UIWidgetObj* obj = mObj->ctx()->alloc_widget(UI_WIDGET_TEXT_EDIT, layoutI, mObj, user);
    obj->as.textEdit.fontSize = widgetI.fontSize;
    obj->as.textEdit.buf = TextBuffer<char>::create();
    obj->as.textEdit.domain = widgetI.domain;
    obj->as.textEdit.onChange = widgetI.onChange;
    obj->as.textEdit.onSubmit = widgetI.onSubmit;
    obj->flags |= UI_WIDGET_FLAG_FOCUSABLE_BIT;
    obj->cb.onEvent = &UITextEditWidgetObj::on_event;
    obj->cb.onDraw = &UITextEditWidget::on_draw;

    return {obj};
}

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

//
// UIPanelWidget
//

UIPanelWidget UINode::add_panel(const UILayoutInfo& layoutI, const UIPanelWidgetInfo& widgetI, void* user)
{
    UIWidgetObj* obj = mObj->ctx()->alloc_widget(UI_WIDGET_PANEL, layoutI, mObj, user);
    obj->as.panel.color = widgetI.color;

    return {obj};
}

Color* UIPanelWidget::panel_color()
{
    return &mObj->as.panel.color;
}

void UIPanelWidget::on_draw(UIWidget widget, ScreenRenderComponent renderer)
{
    UIWidgetObj* obj = widget;
    UIPanelWidgetObj& self = obj->as.panel;
    Rect rect = widget.get_rect();

    renderer.draw_rect(rect, self.color);
}

//
// UIToggleWidget
//

bool UIToggleWidgetObj::on_event(UIWidget widget, const UIEvent& event)
{
    UIWidgetObj* obj = (UIWidgetObj*)widget;
    UIToggleWidgetObj& self = obj->as.toggle;

    if (event.type == UI_EVENT_MOUSE_DOWN)
    {
        self.state = !self.state;
        self.anim.set(0.32f);

        if (self.user_on_toggle)
            self.user_on_toggle({obj}, self.state, obj->user);

        return true;
    }

    return false;
}

void UIToggleWidgetObj::on_update(UIWidget widget, float delta)
{
    UIWidgetObj* obj = (UIWidgetObj*)widget;
    UIToggleWidgetObj& self = obj->as.toggle;

    // drive toggle animation
    self.anim.update(delta);
}

void UIToggleWidget::on_draw(UIWidget widget, ScreenRenderComponent renderer)
{
    UIWidgetObj* obj = (UIWidgetObj*)widget;
    UITheme theme = widget.get_theme();
    UIToggleWidgetObj& self = obj->as.toggle;
    Rect rect = widget.get_rect();

    renderer.draw_rect(rect, theme.get_background_color());

    rect.w /= 2.0f;

    // animate position
    float ratio = self.anim.get();
    if (!self.state)
        ratio = 1.0f - ratio;

    rect.x += rect.w * ratio;

    uint32_t color = theme.get_on_surface_color();
    if (widget.is_pressed())
    {
        color &= ~0xFF;
        color |= 200;
    }
    else if (widget.is_hovered())
    {
        color &= ~0xFF;
        color |= 234;
    }
    renderer.draw_rect(rect, color);
}

//
// UIButtonWidget
//

void UIButtonWidgetObj::cleanup(UIWidgetObj* base)
{
    UIButtonWidgetObj& self = base->as.button;

    if (self.text)
    {
        heap_free((void*)self.text);
        self.text = nullptr;
    }
}

bool UIButtonWidgetObj::on_event(UIWidget widget, const UIEvent& event)
{
    UIWidgetObj* obj = widget;
    UIButtonWidgetObj& self = obj->as.button;

    // TODO: click semantics, usually when MOUSE_UP event is still within the button rect.
    if (event.type == UI_EVENT_MOUSE_DOWN && self.onClick)
    {
        self.onClick((UIButtonWidget)widget, event.mouse.button, obj->user);
        return true;
    }

    return false;
}

void UIButtonWidget::on_draw(UIWidget widget, ScreenRenderComponent renderer)
{
    UIWidgetObj* obj = widget;
    UIContextObj* ctx = obj->ctx();
    UIButtonWidgetObj& self = obj->as.button;
    UITheme theme = widget.get_theme();
    Rect rect = widget.get_rect();
    Color color = theme.get_selection_color();

    if (widget.is_pressed())
        color = Color::darken(color, 0.05f);
    else if (widget.is_hovered())
        color = Color::lift(color, 0.07f);

    if (!self.transparentBG)
        renderer.draw_rect(rect, color);

    if (self.text)
    {
        float fontSize = rect.h * 0.8f;
        FontAtlas atlas = ctx->fontAtlas;
        Font font = atlas.get_font();
        FontMetrics metrics;
        RImage atlasImage = ctx->fontAtlasImage;
        font.get_metrics(metrics, fontSize);
        Vec2 baseline = rect.get_pos();
        baseline.y += metrics.ascent;

        float advanceX;
        float textWidth = 0.0f;
        Rect glyphBB;
        size_t len = strlen(self.text);

        for (size_t i = 0; i < len; i++)
        {
            uint32_t code = (uint32_t)self.text[i];
            atlas.get_baseline_glyph(code, fontSize, baseline, glyphBB, advanceX);
            textWidth += advanceX;
        }

        baseline.x += (rect.w - textWidth) / 2.0f;

        for (size_t i = 0; i < len; i++)
        {
            uint32_t code = (uint32_t)self.text[i];
            atlas.get_baseline_glyph(code, fontSize, baseline, glyphBB, advanceX);

            Color textColor = self.textColor ? self.textColor : theme.get_on_surface_color();
            renderer.draw_glyph_baseline(atlas, atlasImage, fontSize, baseline, code, textColor);

            baseline.x += advanceX;
        }
    }
}

const char* UIButtonWidget::get_button_text()
{
    return mObj->as.button.text;
}

void UIButtonWidget::set_button_text(const char* text)
{
    UIButtonWidgetObj& self = mObj->as.button;

    if (self.text)
    {
        heap_free((void*)self.text);
        self.text = nullptr;
    }

    self.text = heap_strdup(text, MEMORY_USAGE_UI);
}

void UIButtonWidget::set_on_click(void (*onClick)(UIButtonWidget w, MouseButton btn, void* user))
{
    mObj->as.button.onClick = onClick;
}

bool UIToggleWidget::get_state()
{
    return mObj->as.toggle.state;
}

void ui_obj_cleanup(UIWidgetObj* widget)
{
    LD_ASSERT(widget);

    if (!sWidgetTable[widget->type].cleanup)
        return;

    sWidgetTable[widget->type].cleanup(widget);
}

const char* get_ui_widget_type_cstr(UIWidgetType type)
{
    return sWidgetTable[(int)type].typeName;
}

bool get_ui_widget_type_from_cstr(UIWidgetType& outType, const char* cstr)
{
    if (!cstr)
        return false;

    outType = UI_WIDGET_TYPE_COUNT;

    for (int i = 0; i < (int)UI_WIDGET_TYPE_COUNT; i++)
    {
        if (!strcmp(cstr, sWidgetTable[i].typeName))
        {
            outType = (UIWidgetType)i;
            return true;
        }
    }

    return false;
}

} // namespace LD