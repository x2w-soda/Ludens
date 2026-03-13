#include "UIButtonWidgetObj.h"
#include "../UIContextObj.h"
#include "../UIWidgetObj.h"

namespace LD {

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

} // namespace LD