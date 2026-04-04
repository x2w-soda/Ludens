#pragma once

#include <Ludens/UI/Widget/UIPanelWidget.h>

namespace LD {

struct UIWidgetObj;

struct UIPanelWidgetObj
{
    UIWidgetObj* base;
    UIPanelStorage* storage;
    UIPanelStorage local;

    static void startup(UIWidgetObj* obj, void* storage);
    static void cleanup(UIWidgetObj* obj);
    static void on_draw(UIWidgetObj* obj, ScreenRenderComponent renderer);
};

} // namespace LD