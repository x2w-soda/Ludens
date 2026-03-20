#pragma once

#include <Ludens/Text/TextBuffer.h>
#include <Ludens/UI/Widget/UITextEditWidget.h>

namespace LD {

struct UITextEditWidgetObj
{
    UIWidgetObj* base;
    UITextEditStorage* storage;
    UITextEditStorage local;
    UITextEditOnChange onChange = nullptr;
    UITextEditOnSubmit onSubmit = nullptr;
    bool isEditing;

    void domain_string_on_key(const UIEvent& event, bool& hasChanged, bool& hasSubmitted);
    void domain_uint_on_key(const UIEvent& event, bool& hasChanged, bool& hasSubmitted);

    static void startup(UIWidgetObj* base, void* storage);
    static void cleanup(UIWidgetObj* base);
    static bool on_event(UIWidget widget, const UIEvent& event);
};

} // namespace LD