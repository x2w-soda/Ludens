#pragma once

#include <Ludens/Header/Color.h>
#include <Ludens/Media/Font.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/Text/TextSpan.h>
#include <Ludens/UI/Widget/UITextWidget.h>

#include "../UILibDef.h"

namespace LD {

struct UITextWidgetObj : UIWidgetBaseObj<UITextData>
{
    void set_text_style(Color color, TextSpanFont font);
    void update_span_index(Vec2 localPos);

    static UILayoutInfo default_layout();
    static void startup(UIWidgetObj* obj);
    static void cleanup(UIWidgetObj* obj);
    static bool on_event(UIWidgetObj* obj, const UIEvent& event);
    static void on_draw(UIWidgetObj* obj, ScreenRenderComponent renderer);
    static void wrap_limit(UIWidgetObj* obj, float& outMinW, float& outMaxW);
    static float wrap_size(UIWidgetObj* obj, float limitW);
};

} // namespace LD