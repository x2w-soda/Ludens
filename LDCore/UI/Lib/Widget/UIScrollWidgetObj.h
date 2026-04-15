#pragma once

#include <Ludens/UI/Widget/UIScrollWidget.h>

#include "../UILibDef.h"

namespace LD {

struct UIScrollWidgetObj : UIWidgetBaseObj<UIScrollData>
{
    void set_offset_dst_x(float dstX, bool snap);
    void set_offset_dst_y(float dstY, bool snap);

    static UILayoutInfo default_layout();
    static void startup(UIWidgetObj* obj);
    static void cleanup(UIWidgetObj* obj);
    static bool on_event(UIWidgetObj* obj, const UIEvent& event);
    static void on_update(UIWidgetObj* obj, float delta);
    static void on_draw(UIWidgetObj* obj, ScreenRenderComponent renderer);
};

} // namespace LD