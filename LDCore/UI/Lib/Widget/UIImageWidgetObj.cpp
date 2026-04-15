#include <Ludens/UI/Widget/UIImageWidget.h>

#include "../UIContextObj.h"
#include "../UIWidgetObj.h"
#include "UIImageWidgetObj.h"

namespace LD {

void UIImageWidgetObj::startup(UIWidgetObj* obj)
{
    UIImageWidgetObj& self = obj->U->image;
    new (&self) UIImageWidgetObj();
    self.connect(obj);
}

void UIImageWidgetObj::cleanup(UIWidgetObj* obj)
{
    UIImageWidgetObj& self = obj->U->image;

    (&self)->~UIImageWidgetObj();
}

void UIImageWidgetObj::on_draw(UIWidgetObj* obj, ScreenRenderComponent renderer)
{
    UIImageWidgetObj& self = obj->U->image;
    UIImageData& data = self.get_data();
    const Rect& rect = self.get_rect();
    const Rect& imageRect = data.rect;
    float imageW = (float)data.image.width();
    float imageH = (float)data.image.height();

    if (imageRect.w <= 0.0f)
    {
        renderer.draw_image(rect, data.tint, data.image, Rect(0.0f, 0.0f, 1.0f, 1.0f), false);
    }
    else
    {
        Rect uv = imageRect;
        uv.x /= imageW;
        uv.y /= imageH;
        uv.w /= imageW;
        uv.h /= imageH;
        renderer.draw_image(rect, data.tint, data.image, uv, false);
    }
}

void UIImageWidget::on_draw(UIWidget widget, ScreenRenderComponent renderer)
{
    UIImageWidgetObj::on_draw(widget.unwrap(), renderer);
}

} // namespace LD