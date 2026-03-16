#pragma once

#include <Ludens/Header/Color.h>
#include <Ludens/Header/Math/Rect.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/UI/Widget/UIImageWidget.h>

namespace LD {

struct UIWidgetObj;

struct UIImageWidgetObj
{
    UIWidgetObj* base;
    UIImageStorage* storage;
    UIImageStorage local;

    static void startup(UIWidgetObj* obj, void* storage);
    static void cleanup(UIWidgetObj* obj);
};

} // namespace LD