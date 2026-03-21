#include <Ludens/UI/Widget/UIButtonWidget.h>

#include "../UIContextObj.h"
#include "../UIWidgetObj.h"
#include "UIButtonWidgetObj.h"

namespace LD {

void UIButtonWidgetObj::startup(UIWidgetObj* obj, void* storage)
{
    UIButtonWidgetObj& self = obj->as.button;
    new (&self) UIButtonWidgetObj();

    self.base = obj;
    self.storage = (UIButtonStorage*)storage;

    if (!self.storage)
    {
        obj->flags |= UI_WIDGET_FLAG_LOCAL_STORAGE_BIT;
        self.storage = &self.local;
    }

    obj->cb.onEvent = UIButtonWidgetObj::on_event;

    UIButtonWidget handle{obj};
}
void UIButtonWidgetObj::cleanup(UIWidgetObj* base)
{
    UIButtonWidgetObj& self = base->as.button;

    (&self)->~UIButtonWidgetObj();
}

bool UIButtonWidgetObj::on_event(UIWidget widget, const UIEvent& event)
{
    UIWidgetObj* obj = widget;
    UIButtonWidgetObj& self = obj->as.button;

    // TODO: click semantics, usually when MOUSE_UP event is still within the button rect.
    if (event.type == UI_EVENT_MOUSE_DOWN && self.onClick)
    {
        self.onClick(widget, event.mouse.button, obj->user);
        return true;
    }

    return false;
}

void UIButtonWidget::on_draw(UIWidget widget, ScreenRenderComponent renderer)
{
    UIWidgetObj* obj = widget;
    UIContextObj* ctx = obj->ctx();
    UIButtonWidgetObj& self = obj->as.button;
    UIButtonStorage* storage = self.storage;
    UITheme theme = widget.get_theme();
    Rect rect = widget.get_rect();
    Color color = theme.get_selection_color();

    if (widget.is_pressed())
        color = Color::darken(color, 0.05f);
    else if (widget.is_hovered())
        color = Color::lift(color, 0.07f);

    if (!storage->transparentBG)
        renderer.draw_rect(rect, color);

    if (!storage->text.empty())
    {
        float fontSize = rect.h * 0.8f;
        FontAtlas atlas = ctx->fontDefault.font_atlas();
        Font font = atlas.get_font();
        FontMetrics metrics;
        RImage atlasImage = ctx->fontDefault.image();
        font.get_metrics(metrics, fontSize);
        Vec2 baseline = rect.get_pos();
        baseline.y += metrics.ascent;

        float advanceX;
        float textWidth = 0.0f;
        Rect glyphBB;
        size_t len = storage->text.size();

        for (size_t i = 0; i < len; i++)
        {
            uint32_t code = (uint32_t)storage->text[i];
            atlas.get_baseline_glyph(code, fontSize, baseline, glyphBB, advanceX);
            textWidth += advanceX;
        }

        baseline.x += (rect.w - textWidth) / 2.0f;

        for (size_t i = 0; i < len; i++)
        {
            uint32_t code = (uint32_t)storage->text[i];
            atlas.get_baseline_glyph(code, fontSize, baseline, glyphBB, advanceX);

            Color textColor = storage->textColor ? storage->textColor : theme.get_on_surface_color();
            renderer.draw_glyph_baseline(atlas, atlasImage, fontSize, baseline, code, textColor);

            baseline.x += advanceX;
        }
    }
}

UIButtonStorage* UIButtonWidget::get_storage()
{
    return mObj->as.button.storage;
}

void UIButtonWidget::set_on_click(UIButtonOnClick onClick)
{
    mObj->as.button.onClick = onClick;
}

} // namespace LD