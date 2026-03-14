#pragma once

namespace LD {

struct UIWidgetObj;

struct UIPanelWidgetObj
{
    UIWidgetObj* base;
    UIPanelStorage* storage;

    static void startup(UIWidgetObj* obj, void* storage);
    static void cleanup(UIWidgetObj* obj);
};

} // namespace LD