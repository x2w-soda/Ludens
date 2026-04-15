#include <Ludens/UI/Widget/UIPanelWidget.h>

#include "../UIWidgetObj.h"

namespace LD {

UILayoutInfo UIPanelWidgetObj::default_layout()
{
    UILayoutInfo layoutI{};
    layoutI.sizeX = UISize::fit();
    layoutI.sizeY = UISize::fit();
    layoutI.childAxis = UI_AXIS_Y;

    return layoutI;
}

void UIPanelWidgetObj::startup(UIWidgetObj* obj)
{
    UIPanelWidgetObj& self = obj->U->panel;
    new (&self) UIPanelWidgetObj();
    self.connect(obj);
}

void UIPanelWidgetObj::cleanup(UIWidgetObj* obj)
{
    UIPanelWidgetObj& self = obj->U->panel;

    (&self)->~UIPanelWidgetObj();
}

void UIPanelWidgetObj::on_draw(UIWidgetObj* obj, ScreenRenderComponent renderer)
{
    UIPanelWidgetObj& self = obj->U->panel;
    const UIPanelData& data = self.get_data();

    if (data.color != 0.0f)
    {
        renderer.draw_rect_rounded(self.get_rect(), data.color, data.radius);
    }
}

} // namespace LD