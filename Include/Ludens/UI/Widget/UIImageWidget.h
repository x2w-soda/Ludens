#pragma once

#include <Ludens/UI/UIWidget.h>

namespace LD {

class UIImageData
{
    friend struct UIImageWidgetObj;

public:
    RImage image;            // image to be renderered
    Color tint = 0xFFFFFFFF; // image tint color.
    Rect rect;               // the part of image to be rendered
};

/// @brief UI image widget
struct UIImageWidget : UIWidget
{
    static void on_draw(UIWidget widget, ScreenRenderComponent renderer);
};

} // namespace LD