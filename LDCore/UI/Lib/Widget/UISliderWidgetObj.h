#pragma once

#include <Ludens/UI/Widget/UISliderWidget.h>

namespace LD {

struct UISliderWidgetObj
{
    UIWidgetObj* base;
    UISliderStorage* storage;
    UISliderStorage local;
    Vec2 dragStart;

    inline float get_value()
    {
        return std::lerp(storage->min, storage->max, storage->ratio);
    }

    static void startup(UIWidgetObj* obj, void* storage);
    static void cleanup(UIWidgetObj* obj);
    static bool on_event(UIWidgetObj* obj, const UIEvent& event);
    static void on_draw(UIWidgetObj* obj, ScreenRenderComponent renderer);
};

} // namespace LD