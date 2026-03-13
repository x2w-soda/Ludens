#include "UIImageWidgetObj.h"
#include "../UIContextObj.h"
#include "../UIWidgetObj.h"

namespace LD {

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

} // namespace LD