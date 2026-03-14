#include <Ludens/UI/Widget/UIImageWidget.h>

#include "../UIContextObj.h"
#include "../UIWidgetObj.h"
#include "UIImageWidgetObj.h"

namespace LD {

void UIImageWidgetObj::startup(UIWidgetObj* obj, void* storage)
{
    UIImageWidgetObj& self = obj->as.image;
    new (&self) UIImageWidgetObj();

    self.base = obj;
    self.storage = (UIImageStorage*)storage;
}

void UIImageWidgetObj::cleanup(UIWidgetObj* obj)
{
    UIImageWidgetObj& self = obj->as.image;

    (&self)->~UIImageWidgetObj();
}

void UIImageWidget::on_draw(UIWidget widget, ScreenRenderComponent renderer)
{
    UIImageWidgetObj& self = static_cast<UIWidgetObj*>(widget)->as.image;
    UIImageStorage* storage = self.storage;
    const Rect& imageRect = storage->rect;
    Rect rect = widget.get_rect();
    RImage image = storage->image;
    float imageW = (float)image.width();
    float imageH = (float)image.height();

    if (imageRect.w <= 0.0f)
    {
        renderer.draw_image(rect, storage->tint, image, Rect(0.0f, 0.0f, 1.0f, 1.0f), false);
    }
    else
    {
        Rect uv = imageRect;
        uv.x /= imageW;
        uv.y /= imageH;
        uv.w /= imageW;
        uv.h /= imageH;
        renderer.draw_image(rect, storage->tint, image, uv, false);
    }
}

} // namespace LD