#pragma once

#include <Ludens/UI/Widget/UISliderWidget.h>

namespace LD {

struct UISliderWidgetObj
{
    UIWidgetObj* base;
    UISliderStorage* storage;
    Vec2 dragStart;

    inline float get_value()
    {
        return std::lerp(storage->min, storage->max, storage->ratio);
    }

    static void startup(UIWidgetObj* obj, void* storage);
    static void cleanup(UIWidgetObj* obj);
    static bool on_event(UIWidget widget, const UIEvent& event);
};

} // namespace LD