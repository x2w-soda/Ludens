#pragma once

#include <Ludens/RenderComponent/ScreenRenderComponent.h>
#include <Ludens/UI/UIWidget.h>

namespace LD {

struct UIWidgetMeta
{
    UIWidgetType type;
    const char* typeName;
    size_t objSize;
    void (*startup)(UIWidgetObj* obj, void* storage);
    void (*cleanup)(UIWidgetObj* obj);
    void (*onDraw)(UIWidget widget, ScreenRenderComponent renderer);
};

extern UIWidgetMeta sWidgetMeta[];

void widget_startup(UIWidgetObj* obj, void* storage);
void widget_cleanup(UIWidgetObj* obj);
void widget_on_draw(UIWidget widget, ScreenRenderComponent renderer);

} // namespace LD