#include <Ludens/UI/Widget/UIPanelWidget.h>

#include "../UIWidgetObj.h"

namespace LD {

void UIPanelWidgetObj::startup(UIWidgetObj* obj, void* storage)
{
    UIPanelWidgetObj& self = obj->as.panel;
    new (&self) UIPanelWidgetObj();

    self.base = obj;
    self.storage = (UIPanelStorage*)storage;

    if (!self.storage)
    {
        obj->flags |= UI_WIDGET_FLAG_LOCAL_STORAGE_BIT;
        self.storage = &self.local;
    }
}

void UIPanelWidgetObj::cleanup(UIWidgetObj* obj)
{
    UIPanelWidgetObj& self = obj->as.panel;

    (&self)->~UIPanelWidgetObj();
}

UIPanelStorage* UIPanelWidget::get_storage()
{
    return mObj->as.panel.storage;
}

void UIPanelWidget::on_draw(UIWidget widget, ScreenRenderComponent renderer)
{
    UIWidgetObj* obj = widget;
    UIPanelWidgetObj& self = obj->as.panel;
    Rect rect = widget.get_rect();

    renderer.draw_rect(rect, self.storage->color);
}

} // namespace LD