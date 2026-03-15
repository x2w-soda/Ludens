#pragma once

#include <Ludens/UI/UIWidget.h>

#include <string>

namespace LD {

typedef void (*UIButtonOnClick)(UIWidget w, MouseButton btn, void* user);

struct UIButtonStorage
{
    std::string text;
    Color textColor;
    bool transparentBG;
};

/// @brief UI button widget.
struct UIButtonWidget : UIWidget
{
    UIButtonStorage* get_storage();

    void set_on_click(UIButtonOnClick onClick);

    /// @brief Default button widget rendering.
    static void on_draw(UIWidget widget, ScreenRenderComponent renderer);
};

} // namespace LD