#pragma once

#include <Ludens/DSA/Vector.h>
#include <Ludens/Text/TextSpan.h>
#include <Ludens/UI/UIFont.h>
#include <Ludens/UI/UIWidget.h>

#include <string>

namespace LD {

struct UITextSpan
{
    TextSpan text = {};
    bool (*onEvent)(UIWidget widget, const UIEvent& event, UITextSpan& span, int spanIndex, void* user) = nullptr;
    void* user = nullptr;
};

struct UITextStorage
{
    std::string value;                    /// text value to display
    Vector<UITextSpan> spans;             /// text spans for rendering, must be synched with value
    float fontSize = UIFont::base_size(); /// rendered font size
    Color bgColor = 0;                    /// background color for entire widget rect

    void set_value(const std::string& newValue);
    void set_value(const std::string& newValue, const Vector<UITextSpan>& newSpans);
    void set_fg_color(Color fgColor);
};

struct UITextWidget : UIWidget
{
    UITextStorage* get_storage();

    /// @brief Set uniform text style for all spans.
    void set_text_style(Color color, TextSpanFont font);

    int get_span_index();
};

} // namespace LD