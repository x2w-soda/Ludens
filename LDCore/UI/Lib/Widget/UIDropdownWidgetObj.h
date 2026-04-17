#pragma once

#include <Ludens/UI/Widget/UIDropdownWidget.h>

#include "../UILibDef.h"

namespace LD {

struct UIDropdownWidgetObj : UIWidgetBaseObj<UIDropdownData>
{
    bool set_option(int index);

    static void startup(UIWidgetObj* base);
    static void cleanup(UIWidgetObj* base);
    static bool on_event(UIWidgetObj* obj, const UIEvent& event);
    static void on_draw(UIWidgetObj* obj, ScreenRenderComponent renderer);
};

} // namespace LD