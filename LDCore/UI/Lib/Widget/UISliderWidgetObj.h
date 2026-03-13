#pragma once

#include <Ludens/UI/UIWidget.h>

namespace LD {

struct UISliderWidgetObj
{
    UIWidgetObj* base;
    Vec2 dragStart;
    float min;
    float max;
    float value;
    float ratio;

    static bool on_event(UIWidget widget, const UIEvent& event);
};

} // namespace LD