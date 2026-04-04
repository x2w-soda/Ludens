#pragma once

#include <Ludens/UI/Widget/UIButtonWidget.h>

namespace LD {

struct UIButtonWidgetObj
{
    UIWidgetObj* base = nullptr;
    UIButtonStorage* storage = nullptr;
    UIButtonStorage local;
    UIButtonOnClick onClick = nullptr;

    static void startup(UIWidgetObj* obj, void* storage);
    static void cleanup(UIWidgetObj* obj);
    static bool on_event(UIWidgetObj* obj, const UIEvent& event);
    static void on_draw(UIWidgetObj* obj, ScreenRenderComponent renderer);
};

} // namespace LD