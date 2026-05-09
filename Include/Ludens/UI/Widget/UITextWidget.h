#pragma once

#include <Ludens/DSA/String.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/Text/TextSpan.h>
#include <Ludens/UI/UIFont.h>
#include <Ludens/UI/UIWidget.h>

namespace LD {

struct UITextSpan;

typedef bool (*UISpanOnEvent)(UIWidget widget, const UIEvent& event, UITextSpan& span, int spanIndex, void* user);

struct UITextSpan
{
    TextSpan text = {};
    UISpanOnEvent onEvent = nullptr;
    void* user = nullptr;
};

class UITextData
{
    friend struct UITextWidgetObj;

public:
    Color bgColor = 0;                    /// background color for entire widget rect
    float fontSize = UIFont::base_size(); /// rendered font size

    void clear_value();
    void set_value(View newValue, Color* color = nullptr);
    void set_value(View newValue, const Vector<UITextSpan>& newSpans);
    void set_fg_color(Color fgColor);
    void set_span_on_event(UISpanOnEvent onEvent, void* user);
    String get_substring(int spanIndex);
    inline int get_span_index() { return mSpanIndex; }
    inline const String& get_value() const { return mValue; }
    inline const Vector<UITextSpan>& get_spans() const { return mSpans; }
    inline Vector<UITextSpan>& get_spans() { return mSpans; }

private:
    String mValue;             /// text value to display
    Vector<UITextSpan> mSpans; /// text spans for rendering, must be synched with value
    int mSpanIndex = -1;
};

struct UITextWidget : UIWidget
{
    /// @brief Set uniform text style for all spans.
    void set_text_style(Color color, TextSpanFont font);

    int get_span_index();
};

} // namespace LD