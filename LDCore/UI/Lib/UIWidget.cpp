#include "UIObj.h"
#include <Ludens/Application/Input.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Types.h>
#include <Ludens/UI/UIWidget.h>
#include <Ludens/UI/UIWindow.h>
#include <algorithm>

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
static_assert(IsTrivial<UIWidgetObj>);
static_assert(IsTrivial<UIScrollWidgetObj>);
static_assert(IsTrivial<UITextWidgetObj>);
static_assert(IsTrivial<UITextEditWidgetObj>);
static_assert(IsTrivial<UIPanelWidgetObj>);
static_assert(IsTrivial<UIImageWidgetObj>);
static_assert(IsTrivial<UIToggleWidgetObj>);
static_assert(IsTrivial<UISliderWidgetObj>);
static_assert(IsTrivial<UIButtonWidgetObj>);

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
    default:
        LD_UNREACHABLE;
    }
}

bool UIWidget::is_hovered()
{
    UIContextObj* ctx = mObj->window->ctx;
    return ctx->cursorWidget == mObj;
}

bool UIWidget::is_pressed()
{
    UIContextObj* ctx = mObj->window->ctx;
    return ctx->pressWidget == mObj;
}

UINode& UIWidget::node()
{
    return mObj->node;
}

void UIWidget::hide()
{
    mObj->flags |= UI_WIDGET_FLAG_HIDDEN_BIT;
}

void UIWidget::show()
{
    mObj->flags &= ~UI_WIDGET_FLAG_HIDDEN_BIT;
}

bool UIWidget::is_hidden()
{
    return static_cast<bool>(mObj->flags & UI_WIDGET_FLAG_HIDDEN_BIT);
}

void UIWidget::block_input()
{
    mObj->flags |= UI_WIDGET_FLAG_BLOCK_INPUT_BIT;

    UIContextObj* ctx = mObj->window->ctx;
    UIContext(ctx).input_mouse_position(ctx->cursorPos);
}

void UIWidget::unblock_input()
{
    mObj->flags &= ~UI_WIDGET_FLAG_BLOCK_INPUT_BIT;

    UIContextObj* ctx = mObj->window->ctx;
    UIContext(ctx).input_mouse_position(ctx->cursorPos);
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
    UIContextObj* ctx = mObj->window->ctx;

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

void UIWidget::set_on_key(void (*onKey)(UIWidget widget, KeyCode key, UIEvent event))
{
    mObj->cb.onKey = onKey;
}

void UIWidget::set_on_mouse(void (*onMouse)(UIWidget widget, const Vec2& pos, MouseButton btn, UIEvent event))
{
    mObj->cb.onMouse = onMouse;
}

void UIWidget::set_on_hover(void (*onHover)(UIWidget widget, UIEvent event))
{
    mObj->cb.onHover = onHover;
}

void UIWidget::set_on_drag(void (*onDrag)(UIWidget widget, MouseButton btn, const Vec2& dragPos, bool begin))
{
    mObj->cb.onDrag = onDrag;
}

void UIWidget::set_on_scroll(void (*onScroll)(UIWidget widget, const Vec2& offset))
{
    mObj->cb.onScroll = onScroll;
}

void UIWidget::set_on_update(void (*onUpdate)(UIWidget widget, float delta))
{
    mObj->cb.onUpdate = onUpdate;
}

void UIWidget::set_on_draw(void (*onDraw)(UIWidget widget, ScreenRenderComponent renderer))
{
    mObj->cb.onDraw = onDraw;
}

UIContextObj* UINode::get_context()
{
    return mObj->window->ctx;
}

void UINode::remove()
{
    UIContextObj* ctx = mObj->window->ctx;

    ctx->free_widget(mObj);
    mObj = nullptr;
}

//
// UIScrollWidget
//

UIScrollWidget UINode::add_scroll(const UILayoutInfo& layoutI, const UIScrollWidgetInfo& widgetI, void* user)
{
    UIWindowObj* window = mObj->window;
    UIWidgetObj* obj = window->ctx->alloc_widget(UI_WIDGET_SCROLL, layoutI, mObj, user);
    obj->as.scroll.base = obj;
    obj->as.scroll.bgColor = widgetI.bgColor;
    obj->as.scroll.offsetXDst = 0.0f;
    obj->as.scroll.offsetXSpeed = 0.0f;
    obj->as.scroll.offsetYDst = 0.0f;
    obj->as.scroll.offsetYSpeed = 0.0f;
    obj->cb.onDraw = &UIScrollWidget::on_draw;
    obj->cb.onUpdate = &UIScrollWidget::on_update;
    obj->cb.onMouse = &UIScrollWidgetObj::on_mouse;
    obj->cb.onScroll = &UIScrollWidgetObj::on_scroll;
    obj->flags |= UI_WIDGET_FLAG_DRAW_WITH_SCISSOR_BIT;

    return {obj};
}

void UIScrollWidgetObj::cleanup(UIWidgetObj* base)
{
    // TODO:
}

void UIScrollWidgetObj::on_scroll(UIWidget widget, const Vec2& offset)
{
    UIWidgetObj* base = (UIWidgetObj*)widget;
    UIScrollWidgetObj& self = base->as.scroll;

    const float sensitivity = 20.0f;
    const float animDuration = 0.14f;

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
}

void UIScrollWidgetObj::on_mouse(UIWidget widget, const Vec2& pos, MouseButton btn, UIEvent event)
{
    // TODO:
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
    UIWindowObj* window = mObj->window;
    UIWidgetObj* obj = window->ctx->alloc_widget(UI_WIDGET_IMAGE, layoutI, mObj, user);
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
        renderer.draw_image(rect, self.imageHandle, self.tint);
    else
    {
        Rect uv = self.imageRect;
        uv.x /= imageW;
        uv.y /= imageH;
        uv.w /= imageW;
        uv.h /= imageH;
        renderer.draw_image_uv(rect, self.imageHandle, uv, self.tint);
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

void UIImageWidget::set_image_tint(Color color)
{
    mObj->as.image.tint = color;
}

//
// UIButtonWidget
//

UIButtonWidget UINode::add_button(const UILayoutInfo& layoutI, const UIButtonWidgetInfo& widgetI, void* user)
{
    UIWindowObj* window = mObj->window;
    UIWidgetObj* obj = window->ctx->alloc_widget(UI_WIDGET_BUTTON, layoutI, mObj, user);
    obj->cb.onMouse = UIButtonWidgetObj::on_mouse;
    obj->cb.onHover = UIButtonWidgetObj::on_hover;
    obj->as.button.base = obj;
    obj->as.button.text = widgetI.text ? heap_strdup(widgetI.text, MEMORY_USAGE_UI) : nullptr;
    obj->as.button.user_on_press = widgetI.on_press;
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
    UIWindowObj* window = mObj->window;
    UIWidgetObj* obj = window->ctx->alloc_widget(UI_WIDGET_SLIDER, layoutI, mObj, user);
    obj->cb.onDrag = &UISliderWidgetObj::on_drag;
    obj->as.slider.base = obj;
    obj->as.slider.min = widgetI.min;
    obj->as.slider.max = widgetI.max;
    obj->as.slider.value = widgetI.min;
    obj->as.slider.ratio = 0.0f;

    return {obj};
}

void UISliderWidgetObj::on_drag(UIWidget widget, MouseButton btn, const Vec2& dragPos, bool begin)
{
    UISliderWidgetObj& self = static_cast<UIWidgetObj*>(widget)->as.slider;

    Rect rect = widget.get_rect();
    self.ratio = std::clamp(((float)dragPos.x - rect.x) / rect.w, 0.0f, 1.0f);
    self.value = std::lerp(self.min, self.max, self.ratio);
}

void UISliderWidget::on_draw(UIWidget widget, ScreenRenderComponent renderer)
{
    UIWidgetObj* obj = widget;
    const UITheme& theme = obj->window->ctx->theme;
    UISliderWidgetObj& self = obj->as.slider;
    Rect rect = widget.get_rect();

    float sliderw = rect.w * 0.1f;
    renderer.draw_rect(rect, theme.get_background_color());

    uint32_t color = theme.get_primary_color();
    if (widget.is_hovered())
    {
        color &= ~0xFF;
        color |= 234;
    }

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
    UIWindowObj* window = mObj->window;
    UIWidgetObj* obj = window->ctx->alloc_widget(UI_WIDGET_TOGGLE, layoutI, mObj, user);
    obj->cb.onMouse = &UIToggleWidgetObj::on_mouse;
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
    if (layoutI.sizeX.type != UI_SIZE_FIXED)
    {
        textLayoutI.sizeX = UISize::wrap_primary();
        textLayoutI.sizeY = UISize::wrap_secondary();
    }

    UIWindowObj* window = mObj->window;
    UIWidgetObj* obj = window->ctx->alloc_widget(UI_WIDGET_TEXT, textLayoutI, mObj, user);
    obj->as.text.fontSize = widgetI.fontSize;
    obj->as.text.value = widgetI.cstr ? heap_strdup(widgetI.cstr, MEMORY_USAGE_UI) : nullptr;
    obj->as.text.fontAtlas = window->ctx->fontAtlas;
    obj->as.text.hoverHL = widgetI.hoverHL;
    obj->as.text.bgColor = 0;

    if (widgetI.hoverHL)
        obj->cb.onHover = [](UIWidget, UIEvent) {}; // widget is hoverable

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
    UIContextObj& ctx = *obj->window->ctx;
    const UITheme& theme = ctx.theme;
    UITextWidgetObj& self = obj->as.text;
    Rect rect = widget.get_rect();
    float wrapWidth = rect.w;

    if (self.bgColor.get_alpha() > 0.0f)
        renderer.draw_rect(rect, self.bgColor);

    if (self.hoverHL && widget.is_hovered())
    {
        renderer.draw_rect(rect, theme.get_on_surface_color());
        renderer.draw_text(ctx.fontAtlas, ctx.fontAtlasImage, self.fontSize, rect.get_pos(), self.value, theme.get_surface_color(), wrapWidth);
    }
    else
    {
        renderer.draw_text(ctx.fontAtlas, ctx.fontAtlasImage, self.fontSize, rect.get_pos(), self.value, theme.get_on_surface_color(), wrapWidth);
    }
}

void UITextWidget::set_text(const char* cstr)
{
    if (mObj->as.text.value)
        heap_free((void*)mObj->as.text.value);

    mObj->as.text.value = cstr ? heap_strdup(cstr, MEMORY_USAGE_UI) : nullptr;
}

//
// UITextEditWidget
//

UITextEditWidget UINode::add_text_edit(const UILayoutInfo& layoutI, const UITextEditWidgetInfo& widgetI, void* user)
{
    UIWindowObj* window = mObj->window;
    UIWidgetObj* obj = window->ctx->alloc_widget(UI_WIDGET_TEXT_EDIT, layoutI, mObj, user);
    obj->as.textEdit.fontSize = widgetI.fontSize;
    obj->as.textEdit.buf = TextBuffer<char>::create();
    obj->cb.onKey = &UITextEditWidgetObj::on_key;
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

void UITextEditWidgetObj::on_key(UIWidget widget, KeyCode keyCode, UIEvent event)
{
    UIWidgetObj* obj = widget.unwrap();
    auto& self = obj->as.textEdit;

    if (event != UI_KEY_DOWN)
        return;

    if (KEY_CODE_A <= keyCode && keyCode <= KEY_CODE_Z)
    {
        char key = (char)keyCode + 32;

        if (Input::get_key(KEY_CODE_LEFT_SHIFT) || Input::get_key(KEY_CODE_RIGHT_SHIFT))
            key -= 32;

        self.buf.push_back(key);
    }
    else if (keyCode == KEY_CODE_SPACE)
    {
        self.buf.push_back(' ');
    }
    else if (keyCode == KEY_CODE_BACKSPACE)
    {
        self.buf.pop_back();
    }
}

void UITextEditWidget::on_draw(UIWidget widget, ScreenRenderComponent renderer)
{
    UIWidgetObj* obj = widget.unwrap();
    auto& self = obj->as.textEdit;
    UIContextObj& ctx = *obj->window->ctx;
    const UITheme& theme = ctx.theme;
    std::string str;

    Rect rect = widget.get_rect();
    renderer.draw_rect(rect, theme.get_field_color());

    if (widget.is_hovered())
        renderer.draw_rect_outline(rect, 1, theme.get_primary_color());

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
    UIWindowObj* window = mObj->window;
    UIWidgetObj* obj = window->ctx->alloc_widget(UI_WIDGET_PANEL, layoutI, mObj, user);
    obj->as.panel.color = widgetI.color;

    return {obj};
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

void UIToggleWidgetObj::on_mouse(UIWidget widget, const Vec2& pos, MouseButton btn, UIEvent event)
{
    UIWidgetObj* obj = (UIWidgetObj*)widget;
    UIToggleWidgetObj& self = obj->as.toggle;

    if (event == UI_MOUSE_DOWN)
    {
        self.state = !self.state;
        self.anim.set(0.32f);

        if (self.user_on_toggle)
            self.user_on_toggle({obj}, self.state, obj->user);
    }
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

void UIButtonWidgetObj::on_mouse(UIWidget widget, const Vec2& pos, MouseButton btn, UIEvent event)
{
    UIWidgetObj* obj = widget;
    UIButtonWidgetObj& self = obj->as.button;

    if (event == UI_MOUSE_DOWN && self.user_on_press)
        self.user_on_press((UIButtonWidget)widget, btn, obj->user);
}

void UIButtonWidgetObj::on_hover(UIWidget widget, UIEvent event)
{
    UIWidgetObj* obj = widget;

    // TODO:
}

void UIButtonWidget::on_draw(UIWidget widget, ScreenRenderComponent renderer)
{
    UIWidgetObj* obj = widget;
    UIContextObj* ctx = obj->window->ctx;
    UIButtonWidgetObj& self = obj->as.button;
    UITheme theme = widget.get_theme();
    Rect rect = widget.get_rect();
    uint32_t color = theme.get_selection_color();

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

bool UIToggleWidget::get_state()
{
    return mObj->as.toggle.state;
}

void UIPanelWidget::set_panel_color(Color color)
{
    mObj->as.panel.color = color;
}

void ui_obj_cleanup(UIWidgetObj* widget)
{
    LD_ASSERT(widget);

    if (!sWidgetTable[widget->type].cleanup)
        return;

    sWidgetTable[widget->type].cleanup(widget);
}

} // namespace LD