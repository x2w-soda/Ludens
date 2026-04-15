#pragma once

#include <Ludens/UI/Widget/UIButtonWidget.h>

#include "../UILibDef.h"

namespace LD {

struct UIButtonWidgetObj : UIWidgetBaseObj<UIButtonData>
{
    void set_on_click(UIButtonOnClick onClick);

    static void startup(UIWidgetObj* obj);
    static void cleanup(UIWidgetObj* obj);
    static bool on_event(UIWidgetObj* obj, const UIEvent& event);
    static void on_draw(UIWidgetObj* obj, ScreenRenderComponent renderer);
};

} // namespace LD