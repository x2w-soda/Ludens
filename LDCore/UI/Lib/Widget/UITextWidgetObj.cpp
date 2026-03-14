#include <Ludens/UI/Widget/UITextWidget.h>

#include "../UIContextObj.h"
#include "../UIWidgetObj.h"
#include "UITextWidgetObj.h"

namespace LD {

void UITextWidgetObj::startup(UIWidgetObj* obj, void* storage)
{
    UIContextObj* ctx = obj->ctx();
    auto& self = obj->as.text;

    self.storage = (UITextStorage*)storage;
    self.storage->fgColor = ctx->theme.get_on_surface_color();
    self.fontAtlas = ctx->fontAtlas;
    self.fontImage = ctx->fontAtlasImage;
}

void UITextWidgetObj::cleanup(UIWidgetObj* base)
{
    UITextWidgetObj& self = base->as.text;

    self.~UITextWidgetObj();
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

    renderer.draw_text(self.fontAtlas, self.fontImage, storage->fontSize, rect.get_pos(), storage->value.c_str(), storage->fgColor, wrapWidth);
}

void UITextWidget::set_text_style(Color color, FontAtlas fontAtlas, RImage fontImage)
{
    mObj->as.text.storage->fgColor = color;

    if (fontAtlas && fontImage)
    {
        mObj->as.text.fontAtlas = fontAtlas;
        mObj->as.text.fontImage = fontImage;
    }
}

} // namespace LD