#pragma once

#include <Ludens/UI/UIWidget.h>

#include <string>

namespace LD {

typedef void (*UIButtonOnClick)(UIWidget w, MouseButton btn, void* user);

/// @brief UI button widget.
struct UIButtonWidget : UIWidget
{
    /// @brief Default button widget rendering.
    static void on_draw(UIWidget widget, ScreenRenderComponent renderer);

    void set_on_click(UIButtonOnClick onClick);
};

struct UIButtonStorage
{
    std::string text;
    Color textColor;
    bool transparentBG;
};

} // namespace LD