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

void UIPanelWidgetObj::on_draw(UIWidgetObj* obj, ScreenRenderComponent renderer)
{
    UIPanelWidgetObj& self = obj->as.panel;
    UIPanelStorage& storage = *self.storage;

    if (storage.color != 0.0f)
    {
        renderer.draw_rect_rounded(obj->layout.rect, storage.color, storage.radius);
    }
}

UIPanelStorage* UIPanelWidget::get_storage()
{
    return mObj->as.panel.storage;
}

void UIPanelWidget::set_storage(UIPanelStorage* storage)
{
    mObj->as.panel.storage = storage;
}

} // namespace LD