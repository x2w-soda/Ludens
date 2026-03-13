#pragma once

#include <Ludens/UI/UIWidget.h>

namespace LD {

struct UIButtonWidgetObj
{
    UIWidgetObj* base;
    const char* text;
    void (*onClick)(UIButtonWidget w, MouseButton btn, void* user);
    Color textColor;
    bool transparentBG;

    static void cleanup(UIWidgetObj* base);
    static bool on_event(UIWidget widget, const UIEvent& event);
};

} // namespace LD