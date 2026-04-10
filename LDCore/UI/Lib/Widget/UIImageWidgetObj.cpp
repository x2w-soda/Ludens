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

    if (!self.storage)
    {
        obj->flags |= UI_WIDGET_FLAG_LOCAL_STORAGE_BIT;
        self.storage = &self.local;
    }
}

void UIImageWidgetObj::cleanup(UIWidgetObj* obj)
{
    UIImageWidgetObj& self = obj->as.image;

    (&self)->~UIImageWidgetObj();
}

void UIImageWidgetObj::on_draw(UIWidgetObj* obj, ScreenRenderComponent renderer)
{
    UIImageWidgetObj& self = obj->as.image;
    UIImageStorage* storage = self.storage;
    UIWidget widget(obj);
    const Rect& imageRect = storage->rect;
    const Rect& rect = obj->layout.rect;
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

UIImageStorage* UIImageWidget::get_storage()
{
    return mObj->as.image.storage;
}

void UIImageWidget::set_storage(UIImageStorage* storage)
{
    mObj->as.image.storage = storage;
}

void UIImageWidget::on_draw(UIWidget widget, ScreenRenderComponent renderer)
{
    UIImageWidgetObj::on_draw(widget.unwrap(), renderer);
}

} // namespace LD