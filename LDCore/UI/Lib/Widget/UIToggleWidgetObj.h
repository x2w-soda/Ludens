#pragma once

#include <Ludens/UI/UIAnimation.h>
#include <Ludens/UI/Widget/UIToggleWidget.h>

namespace LD {

struct UIWidgetObj;

struct UIToggleWidgetObj
{
    UIWidgetObj* base;
    UIToggleStorage* storage;
    UIToggleStorage local;
    UIToggleOnToggle onToggle = nullptr;
    UIAnimation<QuadraticInterpolation> anim;

    static void startup(UIWidgetObj* obj, void* storage);
    static void cleanup(UIWidgetObj* obj);
    static bool on_event(UIWidgetObj* obj, const UIEvent& event);
    static void on_update(UIWidgetObj* obj, float delta);
    static void on_draw(UIWidgetObj* obj, ScreenRenderComponent renderer);
};

} // namespace LD