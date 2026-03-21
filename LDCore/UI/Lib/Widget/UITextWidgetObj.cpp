#include <Ludens/UI/Widget/UITextWidget.h>

#include "../UIContextObj.h"
#include "../UIWidgetObj.h"
#include "UITextWidgetObj.h"

namespace LD {

void UITextStorage::set_value(const std::string& newValue)
{
    value = newValue;

    spans.resize(1);
    spans[0] = {};
    spans[0].fgColor = 0xFFFFFFFF;
    spans[0].length = (uint32_t)value.size();
}

void UITextStorage::set_value(const std::string& newValue, const Vector<TextSpan>& newSpans)
{
    uint32_t base = 0;

    for (const TextSpan& span : newSpans)
    {
        if (base != span.offset || span.offset + span.length > newValue.size())
        {
            LD_UNREACHABLE;
            return;
        }

        base = span.offset + span.length;
    }

    value = newValue;
    spans = newSpans;
}

void UITextWidgetObj::startup(UIWidgetObj* obj, void* storage)
{
    UIContextObj* ctx = obj->ctx();
    auto& self = obj->as.text;
    new (&self) UITextWidgetObj();

    self.font = ctx->fontDefault;
    self.storage = (UITextStorage*)storage;

    if (!self.storage)
    {
        obj->flags |= UI_WIDGET_FLAG_LOCAL_STORAGE_BIT;
        self.storage = &self.local;
        self.storage->bgColor = 0;
        self.storage->fgColor = ctx->theme.get_on_surface_color();
        self.storage->fontSize = 16.0f;
    }
}

void UITextWidgetObj::cleanup(UIWidgetObj* base)
{
    UITextWidgetObj& self = base->as.text;

    (&self)->~UITextWidgetObj();
}

void UITextWidget::on_draw(UIWidget widget, ScreenRenderComponent renderer)
{
    UIWidgetObj* obj = widget.unwrap();
    UIContextObj& ctx = *obj->ctx();
    const UITheme& theme = obj->theme;
    UITextWidgetObj& self = obj->as.text;
    const Rect& rect = obj->layout.rect;
    const UITextStorage* storage = self.storage;
    float wrapWidth = rect.w;

    if (storage->value.empty() && rect.h == 0) // likely a layout bug in UI text wrapping
        LD_DEBUG_BREAK;

    if (storage->bgColor.get_alpha() > 0.0f)
        renderer.draw_rect(rect, storage->bgColor);

    // TODO: UIFont for each span segment.
    renderer.draw_text(self.font.font_atlas(), self.font.image(), storage->fontSize, rect.get_pos(), storage->value.c_str(), storage->fgColor, wrapWidth);
}

UITextStorage* UITextWidget::get_storage()
{
    return mObj->as.text.storage;
}

void UITextWidget::set_text_style(Color color, UIFont font)
{
    mObj->as.text.storage->fgColor = color;

    if (font)
    {
        mObj->as.text.font = font;
    }
}

} // namespace LD