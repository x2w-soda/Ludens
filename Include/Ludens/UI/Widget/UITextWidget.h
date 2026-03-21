#pragma once

#include <Ludens/DSA/Vector.h>
#include <Ludens/Text/TextSpan.h>
#include <Ludens/UI/UIFont.h>
#include <Ludens/UI/UIWidget.h>
#include <string>

namespace LD {

struct UITextStorage
{
    std::string value;      /// text value
    Vector<TextSpan> spans; /// text spans for rendering, must be synched with value
    float fontSize = 16.0f; /// rendered font size
    Color bgColor = 0;      /// background color for entire widget rect
    Color fgColor = 0;

    void set_value(const std::string& newValue);
    void set_value(const std::string& newValue, const Vector<TextSpan>& newSpans);
};

struct UITextWidget : UIWidget
{
    UITextStorage* get_storage();

    void set_text_style(Color color, UIFont font);

    /// @brief Default text widget rendering.
    static void on_draw(UIWidget widget, ScreenRenderComponent renderer);
};

} // namespace LD