#pragma once

#include <Ludens/UI/UIFont.h>
#include <Ludens/UI/UIWidget.h>

#include <string>

namespace LD {

typedef void (*UIButtonOnClick)(UIWidget w, MouseButton btn, void* user);

struct UIButtonStorage
{
    std::string text;
    UIFont font = {};
    bool transparentBG = false;
    bool isEnabled = true;
};

/// @brief UI button widget.
struct UIButtonWidget : UIWidget
{
    UIButtonStorage* get_storage();

    void set_on_click(UIButtonOnClick onClick);
};

} // namespace LD