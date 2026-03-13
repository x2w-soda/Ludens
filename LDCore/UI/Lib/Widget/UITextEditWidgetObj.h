#pragma once

#include <Ludens/Text/TextBuffer.h>

namespace LD {

struct UITextEditWidgetObj
{
    UIWidgetObj* base;
    TextBuffer<char> buf;
    UITextEditDomain domain;
    const char* placeHolder = nullptr;
    void (*onChange)(UITextEditWidget widget, View text, void* user);
    void (*onSubmit)(UITextEditWidget widget, View text, void* user);
    float fontSize;

    static void cleanup(UIWidgetObj* base);
    static bool on_event(UIWidget widget, const UIEvent& event);

    void domain_string_on_key(const UIEvent& event, bool& hasChanged, bool& hasSubmitted);
    void domain_uint_on_key(const UIEvent& event, bool& hasChanged, bool& hasSubmitted);
};

} // namespace LD