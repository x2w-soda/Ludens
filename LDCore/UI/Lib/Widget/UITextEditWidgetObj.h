#pragma once

#include <Ludens/Text/TextBuffer.h>
#include <Ludens/UI/Widget/UITextEditWidget.h>

#include "../UILibDef.h"

namespace LD {

struct UITextEditDrawInfo;

struct UITextEditWidgetObj : UIWidgetBaseObj<UITextEditData>
{
    void begin_edit();
    void finish_edit();
    void cancel_edit();
    void on_mouse_down_event(const UIEvent& event);
    void on_mouse_drag_event(const UIEvent& event);
    void on_key_down_event(const UIEvent& event);
    void domain_string_on_key(const UIEvent& event, bool& hasChanged, bool& hasSubmitted);
    void domain_uint_on_key(const UIEvent& event, bool& hasChanged, bool& hasSubmitted);
    void domain_f32_on_key(const UIEvent& event, bool& hasChanged, bool& hasSubmitted);
    void draw_edit_state(UITextEditDrawInfo& info);

    static UILayoutInfo default_layout();
    static void startup(UIWidgetObj* base);
    static void cleanup(UIWidgetObj* base);
    static bool on_event(UIWidgetObj* obj, const UIEvent& event);
    static void on_draw(UIWidgetObj* obj, ScreenRenderComponent renderer);
};

} // namespace LD