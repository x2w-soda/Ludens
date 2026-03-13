#include "../UIWidgetObj.h"

namespace LD {

Color* UIPanelWidget::panel_color()
{
    return &mObj->as.panel.color;
}

void UIPanelWidget::on_draw(UIWidget widget, ScreenRenderComponent renderer)
{
    UIWidgetObj* obj = widget;
    UIPanelWidgetObj& self = obj->as.panel;
    Rect rect = widget.get_rect();

    renderer.draw_rect(rect, self.color);
}

} // namespace LD