#pragma once

#include <Ludens/UI/UIAnimation.h>
#include <Ludens/UI/UIWidget.h>

namespace LD {

struct UIWidgetObj;

struct UIToggleWidgetObj
{
    UIWidgetObj* base;
    void (*user_on_toggle)(UIToggleWidget w, bool state, void* user);
    UIAnimation<QuadraticInterpolation> anim;
    bool state;

    static bool on_event(UIWidget widget, const UIEvent& event);
    static void on_update(UIWidget widget, float delta);
};

} // namespace LD