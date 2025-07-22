#include "UIFWidgetObj.h"
#include <Ludens/Header/Assert.h>
#include <Ludens/UIF/UIFWidget.h>
#include <Ludens/UIF/UIFWindow.h>
#include <algorithm>

namespace LD {
namespace UIF {

void Widget::set_on_draw(DrawFn drawFn)
{
    mObj->drawFn = drawFn;
}

bool Widget::is_hovered()
{
    return mObj->handle.is_hovered();
}

bool Widget::is_pressed()
{
    return mObj->handle.is_pressed();
}

void Widget::on_draw(ScreenRenderComponent renderer)
{
    if (mObj->drawFn)
    {
        mObj->drawFn({mObj}, renderer);
        return;
    }
        
    switch (mObj->type)
    {
    case WIDGET_TYPE_PANEL:
        PanelWidgetObj::on_draw(mObj, renderer);
        break;
    case WIDGET_TYPE_BUTTON:
        ButtonWidgetObj::on_draw(mObj, renderer);
        break;
    case WIDGET_TYPE_SLIDER:
        SliderWidgetObj::on_draw(mObj, renderer);
        break;
    case WIDGET_TYPE_TOGGLE:
        ToggleWidgetObj::on_draw(mObj, renderer);
        break;
    case WIDGET_TYPE_IMAGE:
        ImageWidgetObj::on_draw(mObj, renderer);
        break;
    case WIDGET_TYPE_TEXT:
        TextWidgetObj::on_draw(mObj, renderer);
        break;
    default:
        LD_UNREACHABLE;
    }
}

WidgetNode& Widget::node()
{
    return mObj->node;
}

Rect Widget::get_rect() const
{
    return mObj->handle.get_rect();
}

void* Widget::get_user()
{
    return mObj->user;
}

void Widget::set_user(void* user)
{
    mObj->user = user;
}

PanelWidget WidgetNode::add_panel(const UILayoutInfo& layoutI, const PanelWidgetInfo& widgetI, void* user)
{
    WindowObj* window = mObj->window;
    WidgetObj* obj = window->ctx->alloc_widget(WIDGET_TYPE_PANEL, window, user);
    obj->handle = mObj->handle.add_child(layoutI, obj);
    obj->as.panel.color = widgetI.color;

    return {obj};
}

ImageWidget WidgetNode::add_image(const UILayoutInfo& layoutI, const ImageWidgetInfo& widgetI, void* user)
{
    WindowObj* window = mObj->window;
    WidgetObj* obj = window->ctx->alloc_widget(WIDGET_TYPE_IMAGE, window, user);
    obj->handle = mObj->handle.add_child(layoutI, obj);
    obj->as.image.base = obj;
    obj->as.image.imageHandle = widgetI.image;

    return {obj};
}

ButtonWidget WidgetNode::add_button(const UILayoutInfo& layout, const ButtonWidgetInfo& widgetI, void* user)
{
    WindowObj* window = mObj->window;
    WidgetObj* obj = window->ctx->alloc_widget(WIDGET_TYPE_BUTTON, window, user);
    obj->handle = mObj->handle.add_child(layout, obj);
    obj->handle.set_on_press(&ButtonWidgetObj::on_press);
    obj->as.button.base = obj;
    obj->as.button.text = widgetI.text ? heap_strdup(widgetI.text, MEMORY_USAGE_UI) : nullptr;
    obj->as.button.user_on_press = widgetI.on_press;

    ButtonWidget handle{obj};
    return handle;
};

SliderWidget WidgetNode::add_slider(const UILayoutInfo& layoutI, const SliderWidgetInfo& widgetI, void* user)
{
    WindowObj* window = mObj->window;
    WidgetObj* obj = window->ctx->alloc_widget(WIDGET_TYPE_SLIDER, window, user);
    obj->handle = mObj->handle.add_child(layoutI, obj);
    obj->handle.set_on_drag(&SliderWidgetObj::on_drag);
    obj->as.slider.base = obj;
    obj->as.slider.min = widgetI.min;
    obj->as.slider.max = widgetI.max;
    obj->as.slider.value = widgetI.min;
    obj->as.slider.ratio = 0.0f;

    return {obj};
}

ToggleWidget WidgetNode::add_toggle(const UILayoutInfo& layoutI, const ToggleWidgetInfo& widgetI, void* user)
{
    WindowObj* window = mObj->window;
    WidgetObj* obj = window->ctx->alloc_widget(WIDGET_TYPE_TOGGLE, window, user);
    obj->handle = mObj->handle.add_child(layoutI, obj);
    obj->handle.set_on_press(&ToggleWidgetObj::on_press);
    obj->as.toggle.base = obj;
    obj->as.toggle.state = widgetI.state;
    obj->as.toggle.user_on_toggle = widgetI.on_toggle;
    obj->as.toggle.anim.reset(1.0f);

    return {obj};
}

TextWidget WidgetNode::add_text(const UILayoutInfo& layoutI, const TextWidgetInfo& widgetI, void* user)
{
    UILayoutInfo textLayoutI = layoutI;
    textLayoutI.sizeX = UISize::wrap_primary(&TextWidgetObj::wrap_size_fn, &TextWidgetObj::wrap_limit_fn);
    textLayoutI.sizeY = UISize::wrap_secondary();

    WindowObj* window = mObj->window;
    WidgetObj* obj = window->ctx->alloc_widget(WIDGET_TYPE_TEXT, window, user);
    obj->handle = mObj->handle.add_child(textLayoutI, obj);
    obj->as.text.fontSize = widgetI.fontSize;
    obj->as.text.value = widgetI.cstr ? heap_strdup(widgetI.cstr, MEMORY_USAGE_UI) : nullptr;
    obj->as.text.fontAtlas = widgetI.fontAtlas;

    return {obj};
}

void TextWidgetObj::wrap_limit_fn(void* user, float& outMinW, float& outMaxW)
{
    WidgetObj* baseObj = (WidgetObj*)user;
    TextWidgetObj& obj = baseObj->as.text;

    Font font = obj.fontAtlas.get_font();
    FontMetrics metrics;
    font.get_metrics(metrics, obj.fontSize);

    size_t len = strlen(obj.value);
    float lineW = 0.0f;

    outMaxW = 0.0f;
    outMinW = 0.0f;

    if (len == 0)
        return;

    for (size_t i = 0; i < len; i++)
    {
        uint32_t c = (uint32_t)obj.value[i];

        if (c == '\n')
        {
            lineW = 0.0f;
            continue;
        }

        float advanceX;
        Rect rect;
        Vec2 baseline(lineW, (float)metrics.ascent);
        obj.fontAtlas.get_baseline_glyph(c, obj.fontSize, baseline, rect, advanceX);

        lineW += advanceX;
        outMaxW = std::max<float>(outMaxW, lineW);
        outMinW = std::max<float>(outMinW, rect.w);
    }
}

float TextWidgetObj::wrap_size_fn(void* user, float limitW)
{
    WidgetObj* baseObj = (WidgetObj*)user;
    TextWidgetObj& obj = baseObj->as.text;

    Font font = obj.fontAtlas.get_font();
    FontMetrics metrics;
    font.get_metrics(metrics, obj.fontSize);

    Vec2 baseline(0.0f, metrics.ascent);
    size_t len = strlen(obj.value);

    for (size_t i = 0; i < len; i++)
    {
        uint32_t c = (uint32_t)obj.value[i];

        // TODO: text wrapping using whitespace as boundary
        if (c == '\n' || baseline.x >= limitW)
        {
            baseline.y += metrics.lineHeight;
            baseline.x = 0.0f;
            continue;
        }

        float advanceX;
        Rect rect;
        obj.fontAtlas.get_baseline_glyph(c, obj.fontSize, baseline, rect, advanceX);

        baseline.x += advanceX;
    }

    return baseline.y - metrics.descent;
}

void TextWidgetObj::on_draw(WidgetObj* baseObj, ScreenRenderComponent renderer)
{
    ContextObj& ctx = *baseObj->window->ctx;
    const Theme& theme = ctx.theme;
    TextWidgetObj& obj = baseObj->as.text;
    UIElement element = baseObj->handle;
    Rect rect = element.get_rect();
    float wrapWidth = rect.w;

    renderer.draw_text(ctx.fontAtlas, ctx.fontAtlasImage, obj.fontSize, rect.get_pos(), obj.value, theme.onSurfaceColor, wrapWidth);
}

void PanelWidgetObj::on_draw(WidgetObj* baseObj, ScreenRenderComponent renderer)
{
    PanelWidgetObj* obj = &baseObj->as.panel;
    UIElement element = baseObj->handle;
    Rect rect = element.get_rect();

    renderer.draw_rect(rect, obj->color);
}

void ToggleWidgetObj::on_press(void* user, UIElement handle, MouseButton btn)
{
    WidgetObj* baseObj = (WidgetObj*)user;
    ToggleWidgetObj& obj = baseObj->as.toggle;

    obj.state = !obj.state;
    obj.anim.set(0.32f);

    if (obj.user_on_toggle)
        obj.user_on_toggle({baseObj}, obj.state, baseObj->user);
}

void ToggleWidgetObj::on_draw(WidgetObj* baseObj, ScreenRenderComponent renderer)
{
    const Theme& theme = baseObj->window->ctx->theme;
    ToggleWidgetObj& obj = baseObj->as.toggle;
    UIElement element = baseObj->handle;
    Rect rect = element.get_rect();

    renderer.draw_rect(rect, theme.backgroundColor);

    rect.w /= 2.0f;

    // animate position
    float ratio = obj.anim.get();
    if (!obj.state)
        ratio = 1.0f - ratio;

    rect.x += rect.w * ratio;

    uint32_t color = theme.primaryColor;
    if (element.is_pressed())
    {
        color &= ~0xFF;
        color |= 200;
    }
    else if (element.is_hovered())
    {
        color &= ~0xFF;
        color |= 234;
    }
    renderer.draw_rect(rect, color);
}

void ButtonWidgetObj::on_press(void* user, UIElement handle, MouseButton btn)
{
    WidgetObj* baseObj = (WidgetObj*)user;
    ButtonWidgetObj& obj = baseObj->as.button;

    if (obj.user_on_press)
        obj.user_on_press({baseObj}, btn, baseObj->user);
}

void ButtonWidgetObj::on_draw(WidgetObj* baseObj, ScreenRenderComponent renderer)
{
    ContextObj& ctx = *baseObj->window->ctx;
    ButtonWidgetObj& obj = baseObj->as.button;
    const Theme& theme = ctx.theme;
    UIElement element = baseObj->handle;
    Rect rect = element.get_rect();
    uint32_t color = theme.primaryColor;

    if (element.is_pressed())
    {
        color &= ~0xFF;
        color |= 200;
    }
    else if (element.is_hovered())
    {
        color &= ~0xFF;
        color |= 234;
    }

    renderer.draw_rect(rect, color);

    if (obj.text)
    {
        float fontSize = rect.h * 0.8f;
        FontAtlas atlas = ctx.fontAtlas;
        Font font = atlas.get_font();
        FontMetrics metrics;
        RImage atlasImage = ctx.fontAtlasImage;
        font.get_metrics(metrics, fontSize);
        Vec2 baseline = rect.get_pos();
        baseline.y += metrics.ascent;

        float advanceX;
        float textWidth = 0.0f;
        Rect glyphBB;
        size_t len = strlen(obj.text);

        for (size_t i = 0; i < len; i++)
        {
            uint32_t code = (uint32_t)obj.text[i];
            atlas.get_baseline_glyph(code, fontSize, baseline, glyphBB, advanceX);
            textWidth += advanceX;
        }

        baseline.x += (rect.w - textWidth) / 2.0f;

        for (size_t i = 0; i < len; i++)
        {
            uint32_t code = (uint32_t)obj.text[i];
            atlas.get_baseline_glyph(code, fontSize, baseline, glyphBB, advanceX);
            renderer.draw_glyph_baseline(atlas, atlasImage, fontSize, baseline, code, theme.onPrimaryColor);

            baseline.x += advanceX;
        }
    }
}

void SliderWidgetObj::on_drag(void* user, UIElement element, MouseButton btn, const Vec2& dragPos, bool begin)
{
    WidgetObj* baseObj = (WidgetObj*)user;
    SliderWidgetObj* obj = &baseObj->as.slider;

    Rect rect = baseObj->handle.get_rect();
    obj->ratio = std::clamp(((float)dragPos.x - rect.x) / rect.w, 0.0f, 1.0f);
}

void SliderWidgetObj::on_draw(WidgetObj* baseObj, ScreenRenderComponent renderer)
{
    const Theme& theme = baseObj->window->ctx->theme;
    SliderWidgetObj* obj = &baseObj->as.slider;
    UIElement element = baseObj->handle;
    Rect rect = element.get_rect();

    float sliderw = rect.w * 0.1f;
    renderer.draw_rect(rect, theme.backgroundColor);

    uint32_t color = theme.primaryColor;
    if (element.is_hovered())
    {
        color &= ~0xFF;
        color |= 234;
    }

    rect.w = sliderw;
    rect.x += obj->ratio * sliderw * 9.0f;
    renderer.draw_rect(rect, color);
}

float SliderWidget::get_value()
{
    return mObj->as.slider.value;
}

float SliderWidget::get_ratio()
{
    return mObj->as.slider.ratio;
}

bool ToggleWidget::get_state()
{
    return mObj->as.toggle.state;
}

void ImageWidgetObj::on_draw(WidgetObj* baseObj, ScreenRenderComponent renderer)
{
    ImageWidgetObj* obj = &baseObj->as.image;
    UIElement element = baseObj->handle;
    Rect rect = element.get_rect();

    renderer.draw_image(rect, obj->imageHandle);
}

RImage ImageWidget::get_image()
{
    return mObj->as.image.imageHandle;
}

void TextWidget::set_text(const char* cstr)
{
    if (mObj->as.text.value)
        heap_free((void*)mObj->as.text.value);

    mObj->as.text.value = heap_strdup(cstr, MEMORY_USAGE_UI);
}

} // namespace UIF
} // namespace LD