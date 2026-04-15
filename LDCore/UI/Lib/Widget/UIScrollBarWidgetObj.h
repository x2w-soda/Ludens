#pragma once

#include <Ludens/UI/Widget/UIScrollBarWidget.h>

#include "../UILibDef.h"

namespace LD {

struct UIScrollBarWidgetObj : UIWidgetBaseObj<UIScrollBarData>
{
    void on_mouse_drag(const UIEvent& event);
    float get_bar_size_ratio();

    static void startup(UIWidgetObj* obj);
    static void cleanup(UIWidgetObj* obj);
    static void on_update(UIWidgetObj* obj, float delta);
    static bool on_event(UIWidgetObj* obj, const UIEvent& event);
    static void on_draw(UIWidgetObj* obj, ScreenRenderComponent renderer);
};

} // namespace LD