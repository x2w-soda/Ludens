#pragma once

#include <Ludens/UI/Widget/UIPanelWidget.h>

#include "../UILibDef.h"

namespace LD {

struct UIPanelWidgetObj : UIWidgetBaseObj<UIPanelData>
{
    static UILayoutInfo default_layout();
    static void startup(UIWidgetObj* obj);
    static void cleanup(UIWidgetObj* obj);
    static void on_draw(UIWidgetObj* obj, ScreenRenderComponent renderer);
};

} // namespace LD