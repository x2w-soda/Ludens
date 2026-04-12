#pragma once

#include <Ludens/Text/TextBuffer.h>
#include <Ludens/UI/Widget/UITextEditWidget.h>

namespace LD {

struct UITextEditDrawInfo;

struct UITextEditWidgetObj
{
    UIWidgetObj* base;
    UITextEditStorage* storage;
    UITextEditStorage local;
    UITextEditOnChange onChange = nullptr;
    UITextEditOnSubmit onSubmit = nullptr;
    bool isEditing;

    void begin_edit();
    void finish_edit();
    void cancel_edit();
    void on_mouse_down_event(const UIEvent& event);
    void on_key_down_event(const UIEvent& event);
    void domain_string_on_key(const UIEvent& event, bool& hasChanged, bool& hasSubmitted);
    void domain_uint_on_key(const UIEvent& event, bool& hasChanged, bool& hasSubmitted);
    void domain_f32_on_key(const UIEvent& event, bool& hasChanged, bool& hasSubmitted);
    void draw_edit_state(UITextEditDrawInfo& info);

    static void startup(UIWidgetObj* base, void* storage);
    static void cleanup(UIWidgetObj* base);
    static bool on_event(UIWidgetObj* obj, const UIEvent& event);
    static void on_draw(UIWidgetObj* obj, ScreenRenderComponent renderer);
};

} // namespace LD