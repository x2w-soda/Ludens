#pragma once

#include <Ludens/Header/Color.h>
#include <Ludens/Media/Font.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/UI/Widget/UITextWidget.h>

namespace LD {

struct UIWidgetObj;

struct UITextWidgetObj
{
    UIWidgetObj* base;
    UITextStorage* storage;
    UITextStorage local;
    int spanIndex = -1;

    void update_span_index(Vec2 localPos);

    static void startup(UIWidgetObj* obj, void* info);
    static void cleanup(UIWidgetObj* obj);
    static bool on_event(UIWidgetObj* obj, const UIEvent& event);
    static void on_draw(UIWidgetObj* obj, ScreenRenderComponent renderer);
    static void wrap_limit(UIWidgetObj* obj, float& outMinW, float& outMaxW);
    static float wrap_size(UIWidgetObj* obj, float limitW);
};

} // namespace LD