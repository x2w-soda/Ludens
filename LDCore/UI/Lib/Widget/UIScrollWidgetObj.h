#pragma once

#include <Ludens/UI/Widget/UIScrollWidget.h>

namespace LD {

struct UIScrollWidgetObj
{
    UIWidgetObj* base;
    UIScrollStorage* storage;
    float offsetXDst;   // destination value for scrollOffset x
    float offsetYDst;   // destination value for scrollOffset y
    float offsetXSpeed; // animation speed for scrollOffset x
    float offsetYSpeed; // animation speed for scrollOffset y

    static void startup(UIWidgetObj* obj, void* storage);
    static void cleanup(UIWidgetObj* obj);
    static bool on_event(UIWidget widget, const UIEvent& event);
};

} // namespace LD