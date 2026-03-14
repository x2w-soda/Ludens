#pragma once

#include <Ludens/UI/UIWidget.h>
#include <string>

namespace LD {

struct UITextWidget : UIWidget
{
    void set_text_style(Color color, FontAtlas fontAtlas, RImage fontImage);

    /// @brief Default text widget rendering.
    static void on_draw(UIWidget widget, ScreenRenderComponent renderer);
};

struct UITextStorage
{
    std::string value; /// text value
    float fontSize;    /// rendered font size
    Color bgColor;     /// the background color under text
    Color fgColor;     /// the text color
};

} // namespace LD