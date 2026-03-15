#pragma once

#include <Ludens/UI/UIWidget.h>
#include <string>

namespace LD {

struct UITextStorage
{
    std::string value; /// text value
    float fontSize;    /// rendered font size
    Color bgColor;     /// the background color under text
    Color fgColor;     /// the text color
};

struct UITextWidget : UIWidget
{
    UITextStorage* get_storage();

    void set_text_style(Color color, FontAtlas fontAtlas, RImage fontImage);

    /// @brief Default text widget rendering.
    static void on_draw(UIWidget widget, ScreenRenderComponent renderer);
};

} // namespace LD