#pragma once

#include <Ludens/UI/UIWidget.h>

namespace LD {

/// @brief UI slider widget
struct UISliderWidget : UIWidget
{
    /// @brief get slider value
    float get_value();

    /// @brief Default slider widget rendering.
    static void on_draw(UIWidget widget, ScreenRenderComponent renderer);
};

struct UISliderStorage
{
    float min = 0.0f;   /// slider minimum value
    float max = 1.0f;   /// slider maximum value
    float ratio = 1.0f; /// normalized slider ratio
};

} // namespace LD