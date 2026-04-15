#pragma once

#include <Ludens/UI/Widget/UIToggleWidget.h>

#include "../UILibDef.h"

namespace LD {

struct UIToggleWidgetObj : UIWidgetBaseObj<UIToggleData>
{
    static void startup(UIWidgetObj* obj);
    static void cleanup(UIWidgetObj* obj);
    static bool on_event(UIWidgetObj* obj, const UIEvent& event);
    static void on_update(UIWidgetObj* obj, float delta);
    static void on_draw(UIWidgetObj* obj, ScreenRenderComponent renderer);
};

} // namespace LD