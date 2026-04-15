#pragma once

#include <Ludens/RenderComponent/ScreenRenderComponent.h>
#include <Ludens/UI/UIWidget.h>

namespace LD {

struct UIWidgetMeta
{
    UIWidgetType type;
    const char* typeName;
    size_t objSize;
    UILayoutInfo (*defaultLayout)();
    void (*startup)(UIWidgetObj* obj);
    void (*cleanup)(UIWidgetObj* obj);
    bool (*onEvent)(UIWidgetObj* obj, const UIEvent& event);
    void (*onUpdate)(UIWidgetObj* obj, float delta);
    void (*onDraw)(UIWidgetObj* obj, ScreenRenderComponent renderer);
    float (*wrapSizeX)(UIWidgetObj* obj, float limitW);
    void (*wrapLimitX)(UIWidgetObj* obj, float& minWidth, float& maxWidth);
    CursorType (*cursorHint)(UIWidgetObj* obj);
};

extern UIWidgetMeta sWidgetMeta[];

UILayoutInfo widget_default_layout(UIWidgetType type);
CursorType widget_cursor_hint(UIWidgetObj* obj);
void widget_startup(UIWidgetObj* obj);
void widget_cleanup(UIWidgetObj* obj);
void widget_on_update(UIWidgetObj* obj, float delta);
bool widget_on_event(UIWidgetObj* obj, const UIEvent& event);

} // namespace LD