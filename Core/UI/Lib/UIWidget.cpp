#include "UIObj.h"
#include <Ludens/Header/Assert.h>
#include <Ludens/UI/UIWidget.h>
#include <Ludens/UI/UIWindow.h>
#include <algorithm>

namespace LD {

bool UIWidget::is_hovered()
{
    UIContextObj* ctx = mObj->window->ctx;
    return ctx->cursorElement == mObj;
}

bool UIWidget::is_pressed()
{
    UIContextObj* ctx = mObj->window->ctx;
    return ctx->pressElement == mObj;
}

void UIWidget::on_draw(ScreenRenderComponent renderer)
{
    switch (mObj->type)
    {
    case UI_WIDGET_WINDOW:
        if (mObj->cb.onDraw)
            mObj->cb.onDraw(*this, renderer);
        break;
    case UI_WIDGET_PANEL:
        UIPanelWidgetObj::on_draw(*this, renderer);
        break;
    case UI_WIDGET_BUTTON:
        UIButtonWidgetObj::on_draw(*this, renderer);
        break;
    case UI_WIDGET_SLIDER:
        UISliderWidgetObj::on_draw(*this, renderer);
        break;
    case UI_WIDGET_TOGGLE:
        UIToggleWidgetObj::on_draw(*this, renderer);
        break;
    case UI_WIDGET_IMAGE:
        UIImageWidgetObj::on_draw(*this, renderer);
        break;
    case UI_WIDGET_TEXT:
        UITextWidgetObj::on_draw(*this, renderer);
        break;
    default:
        LD_UNREACHABLE;
    }
}

UINode& UIWidget::node()
{
    return mObj->node;
}

Rect UIWidget::get_rect()
{
    return mObj->layout.rect;
}

void* UIWidget::get_user()
{
    return mObj->user;
}

void UIWidget::set_user(void* user)
{
    mObj->user = user;
}

void UIWidget::set_on_key_up(void (*onKeyUp)(UIWidget widget, KeyCode key))
{
    mObj->cb.onKeyUp = onKeyUp;
}

void UIWidget::set_on_key_down(void (*onKeyDown)(UIWidget widget, KeyCode key))
{
    mObj->cb.onKeyDown = onKeyDown;
}

void UIWidget::set_on_mouse_up(void (*onMouseUp)(UIWidget widget, MouseButton btn))
{
    mObj->cb.onMouseUp = onMouseUp;
}

void UIWidget::set_on_mouse_down(void (*onMouseDown)(UIWidget widget, MouseButton btn))
{
    mObj->cb.onMouseDown = onMouseDown;
}

void UIWidget::set_on_enter(void (*onEnter)(UIWidget widget))
{
    mObj->cb.onEnter = onEnter;
}

void UIWidget::set_on_leave(void (*onLeave)(UIWidget widget))
{
    mObj->cb.onLeave = onLeave;
}

void UIWidget::set_on_drag(void (*onDrag)(UIWidget widget, MouseButton btn, const Vec2& dragPos, bool begin))
{
    mObj->cb.onDrag = onDrag;
}

void UIWidget::set_on_update(void (*onUpdate)(UIWidget widget, float delta))
{
    mObj->cb.onUpdate = onUpdate;
}

void UIWidget::set_on_draw(void (*onDraw)(UIWidget widget, ScreenRenderComponent renderer))
{
    mObj->cb.onDraw = onDraw;
}

UIPanelWidget UINode::add_panel(const UILayoutInfo& layoutI, const UIPanelWidgetInfo& widgetI, void* user)
{
    UIWindowObj* window = mObj->window;
    UIWidgetObj* obj = window->ctx->alloc_widget(UI_WIDGET_PANEL, layoutI, mObj, user);
    obj->as.panel.color = widgetI.color;

    return {obj};
}

UIImageWidget UINode::add_image(const UILayoutInfo& layoutI, const UIImageWidgetInfo& widgetI, void* user)
{
    UIWindowObj* window = mObj->window;
    UIWidgetObj* obj = window->ctx->alloc_widget(UI_WIDGET_IMAGE, layoutI, mObj, user);
    obj->as.image.base = obj;
    obj->as.image.imageHandle = widgetI.image;

    return {obj};
}

UIButtonWidget UINode::add_button(const UILayoutInfo& layoutI, const UIButtonWidgetInfo& widgetI, void* user)
{
    UIWindowObj* window = mObj->window;
    UIWidgetObj* obj = window->ctx->alloc_widget(UI_WIDGET_BUTTON, layoutI, mObj, user);
    obj->cb.onMouseDown = UIButtonWidgetObj::on_press;
    obj->as.button.base = obj;
    obj->as.button.text = widgetI.text ? heap_strdup(widgetI.text, MEMORY_USAGE_UI) : nullptr;
    obj->as.button.user_on_press = widgetI.on_press;

    UIButtonWidget handle{obj};
    return handle;
};

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

UIToggleWidget UINode::add_toggle(const UILayoutInfo& layoutI, const UIToggleWidgetInfo& widgetI, void* user)
{
    UIWindowObj* window = mObj->window;
    UIWidgetObj* obj = window->ctx->alloc_widget(UI_WIDGET_TOGGLE, layoutI, mObj, user);
    obj->cb.onMouseDown = &UIToggleWidgetObj::on_press;
    obj->cb.onUpdate = &UIToggleWidgetObj::on_update;
    obj->as.toggle.base = obj;
    obj->as.toggle.state = widgetI.state;
    obj->as.toggle.user_on_toggle = widgetI.on_toggle;
    obj->as.toggle.anim.reset(1.0f);

    return {obj};
}

UITextWidget UINode::add_text(const UILayoutInfo& layoutI, const UITextWidgetInfo& widgetI, void* user)
{
    UILayoutInfo textLayoutI = layoutI;
    textLayoutI.sizeX = UISize::wrap_primary(&UITextWidgetObj::wrap_size_fn, &UITextWidgetObj::wrap_limit_fn);
    textLayoutI.sizeY = UISize::wrap_secondary();

    UIWindowObj* window = mObj->window;
    UIWidgetObj* obj = window->ctx->alloc_widget(UI_WIDGET_TEXT, textLayoutI, mObj, user);
    obj->as.text.fontSize = widgetI.fontSize;
    obj->as.text.value = widgetI.cstr ? heap_strdup(widgetI.cstr, MEMORY_USAGE_UI) : nullptr;
    obj->as.text.fontAtlas = widgetI.fontAtlas;

    return {obj};
}

void UITextWidgetObj::wrap_limit_fn(UIWidgetObj* obj, float& outMinW, float& outMaxW)
{
    UITextWidgetObj& self = obj->as.text;

    Font font = self.fontAtlas.get_font();
    FontMetrics metrics;
    font.get_metrics(metrics, self.fontSize);

    size_t len = strlen(self.value);
    float lineW = 0.0f;

    outMaxW = 0.0f;
    outMinW = 0.0f;

    if (len == 0)
        return;

    for (size_t i = 0; i < len; i++)
    {
        uint32_t c = (uint32_t)self.value[i];

        if (c == '\n')
        {
            lineW = 0.0f;
            continue;
        }

        float advanceX;
        Rect rect;
        Vec2 baseline(lineW, (float)metrics.ascent);
        self.fontAtlas.get_baseline_glyph(c, self.fontSize, baseline, rect, advanceX);

        lineW += advanceX;
        outMaxW = std::max<float>(outMaxW, lineW);
        outMinW = std::max<float>(outMinW, rect.w);
    }
}

float UITextWidgetObj::wrap_size_fn(UIWidgetObj* obj, float limitW)
{
    UITextWidgetObj& self = obj->as.text;

    Font font = self.fontAtlas.get_font();
    FontMetrics metrics;
    font.get_metrics(metrics, self.fontSize);

    Vec2 baseline(0.0f, metrics.ascent);
    size_t len = strlen(self.value);

    for (size_t i = 0; i < len; i++)
    {
        uint32_t c = (uint32_t)self.value[i];

        // TODO: text wrapping using whitespace as boundary
        if (c == '\n' || baseline.x >= limitW)
        {
            baseline.y += metrics.lineHeight;
            baseline.x = 0.0f;
            continue;
        }

        float advanceX;
        Rect rect;
        self.fontAtlas.get_baseline_glyph(c, self.fontSize, baseline, rect, advanceX);

        baseline.x += advanceX;
    }

    return baseline.y - metrics.descent;
}

void UITextWidgetObj::on_draw(UIWidget widget, ScreenRenderComponent renderer)
{
    UIWidgetObj* obj = (UIWidgetObj*)widget;
    UIContextObj& ctx = *obj->window->ctx;
    const UITheme& theme = ctx.theme;
    UITextWidgetObj& self = obj->as.text;
    Rect rect = widget.get_rect();
    float wrapWidth = rect.w;

    renderer.draw_text(ctx.fontAtlas, ctx.fontAtlasImage, self.fontSize, rect.get_pos(), self.value, theme.onSurfaceColor, wrapWidth);
}

void UIPanelWidgetObj::on_draw(UIWidget widget, ScreenRenderComponent renderer)
{
    UIWidgetObj* obj = widget;
    UIPanelWidgetObj& self = obj->as.panel;
    Rect rect = widget.get_rect();

    renderer.draw_rect(rect, self.color);
}

void UIToggleWidgetObj::on_press(UIWidget widget, MouseButton btn)
{
    UIWidgetObj* obj = (UIWidgetObj*)widget;
    UIToggleWidgetObj& self = obj->as.toggle;

    self.state = !self.state;
    self.anim.set(0.32f);

    if (self.user_on_toggle)
        self.user_on_toggle({obj}, self.state, obj->user);
}

void UIToggleWidgetObj::on_update(UIWidget widget, float delta)
{
    UIWidgetObj* obj = (UIWidgetObj*)widget;
    UIToggleWidgetObj& self = obj->as.toggle;

    // drive toggle animation
    self.anim.update(delta);
}

void UIToggleWidgetObj::on_draw(UIWidget widget, ScreenRenderComponent renderer)
{
    UIWidgetObj* obj = (UIWidgetObj*)widget;
    const UITheme& theme = obj->window->ctx->theme;
    UIToggleWidgetObj& self = obj->as.toggle;
    Rect rect = widget.get_rect();

    renderer.draw_rect(rect, theme.backgroundColor);

    rect.w /= 2.0f;

    // animate position
    float ratio = self.anim.get();
    if (!self.state)
        ratio = 1.0f - ratio;

    rect.x += rect.w * ratio;

    uint32_t color = theme.primaryColor;
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

void UIButtonWidgetObj::on_press(UIWidget widget, MouseButton btn)
{
    UIWidgetObj* obj = widget;
    UIButtonWidgetObj& self = obj->as.button;

    if (self.user_on_press)
        self.user_on_press((UIButtonWidget)widget, btn, obj->user);
}

void UIButtonWidgetObj::on_draw(UIWidget widget, ScreenRenderComponent renderer)
{
    UIWidgetObj* obj = widget;
    UIContextObj* ctx = obj->window->ctx;
    UIButtonWidgetObj& self = obj->as.button;
    const UITheme& theme = ctx->theme;
    Rect rect = widget.get_rect();
    uint32_t color = theme.primaryColor;

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
            renderer.draw_glyph_baseline(atlas, atlasImage, fontSize, baseline, code, theme.onPrimaryColor);

            baseline.x += advanceX;
        }
    }
}

void UISliderWidgetObj::on_drag(UIWidget widget, MouseButton btn, const Vec2& dragPos, bool begin)
{
    UISliderWidgetObj& self = static_cast<UIWidgetObj*>(widget)->as.slider;

    Rect rect = widget.get_rect();
    self.ratio = std::clamp(((float)dragPos.x - rect.x) / rect.w, 0.0f, 1.0f);
}

void UISliderWidgetObj::on_draw(UIWidget widget, ScreenRenderComponent renderer)
{
    UIWidgetObj* obj = widget;
    const UITheme& theme = obj->window->ctx->theme;
    UISliderWidgetObj& self = obj->as.slider;
    Rect rect = widget.get_rect();

    float sliderw = rect.w * 0.1f;
    renderer.draw_rect(rect, theme.backgroundColor);

    uint32_t color = theme.primaryColor;
    if (widget.is_hovered())
    {
        color &= ~0xFF;
        color |= 234;
    }

    rect.w = sliderw;
    rect.x += self.ratio * sliderw * 9.0f;
    renderer.draw_rect(rect, color);
}

float UISliderWidget::get_value()
{
    return mObj->as.slider.value;
}

float UISliderWidget::get_ratio()
{
    return mObj->as.slider.ratio;
}

bool UIToggleWidget::get_state()
{
    return mObj->as.toggle.state;
}

void UIImageWidgetObj::on_draw(UIWidget widget, ScreenRenderComponent renderer)
{
    UIImageWidgetObj& self = static_cast<UIWidgetObj*>(widget)->as.image;
    Rect rect = widget.get_rect();

    renderer.draw_image(rect, self.imageHandle);
}

RImage UIImageWidget::get_image()
{
    return mObj->as.image.imageHandle;
}

void UITextWidget::set_text(const char* cstr)
{
    if (mObj->as.text.value)
        heap_free((void*)mObj->as.text.value);

    // TODO: fix memory leak, last strdup is never freed
    mObj->as.text.value = heap_strdup(cstr, MEMORY_USAGE_UI);
}

} // namespace LD