#pragma once

#include <Ludens/DSA/String.h>
#include <Ludens/UI/UIFont.h>
#include <Ludens/UI/UIWidget.h>

namespace LD {

typedef void (*UIButtonOnClick)(UIWidget w, MouseButton btn, void* user);

class UIButtonData
{
    friend struct UIButtonWidgetObj;

public:
    String text;
    UIFont font = {};
    bool transparentBG = false;
    bool isEnabled = true;
    float radius = 0.2f;

private:
    UIButtonOnClick mOnClick = nullptr;
};

/// @brief UI button widget.
struct UIButtonWidget : UIWidget
{
    void set_on_click(UIButtonOnClick onClick);
};

} // namespace LD