#pragma once

#include <Ludens/UI/Widget/UIButtonWidget.h>

namespace LD {

struct UIButtonWidgetObj
{
    UIWidgetObj* base;
    UIButtonStorage* storage;
    UIButtonStorage local;
    UIButtonOnClick onClick = nullptr;

    static void startup(UIWidgetObj* obj, void* storage);
    static void cleanup(UIWidgetObj* obj);
    static bool on_event(UIWidget widget, const UIEvent& event);
};

} // namespace LD