#pragma once

#include <Ludens/UI/UIWidget.h>

namespace LD {

struct UIScrollWidgetObj
{
    UIWidgetObj* base;
    float offsetXDst;   // destination value for scrollOffset x
    float offsetYDst;   // destination value for scrollOffset y
    float offsetXSpeed; // animation speed for scrollOffset x
    float offsetYSpeed; // animation speed for scrollOffset y
    Color bgColor;
    bool hasScrollBar;

    static void cleanup(UIWidgetObj* base);
    static bool on_event(UIWidget widget, const UIEvent& event);
};

} // namespace LD