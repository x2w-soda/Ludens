#pragma once

#include <Ludens/UI/UIWidget.h>

namespace LD {

struct UISliderWidgetObj
{
    UIWidgetObj* base;
    UISliderStorage* storage;
    Vec2 dragStart;
    float value;

    static void startup(UIWidgetObj* obj, void* storage);
    static void cleanup(UIWidgetObj* obj);
    static bool on_event(UIWidget widget, const UIEvent& event);
};

} // namespace LD