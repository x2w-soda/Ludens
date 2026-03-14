#pragma once

#include <Ludens/UI/UIWidget.h>

namespace LD {

/// @brief UI image widget
struct UIImageWidget : UIWidget
{
    /// @brief Default image widget rendering.
    static void on_draw(UIWidget widget, ScreenRenderComponent renderer);
};

struct UIImageStorage
{
    RImage image;            // image to be renderered
    Color tint = 0xFFFFFFFF; // image tint color.
    Rect rect;               // the part of image to be rendered
};

} // namespace LD