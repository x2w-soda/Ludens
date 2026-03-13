#include "UITextWidgetObj.h"
#include "../UIWidgetObj.h"

namespace LD {

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

} // namespace LD