#pragma once

#include <Ludens/UI/UIWidget.h>

namespace LD {

struct UIScrollWidget : UIWidget
{
    /// @brief Set scroll offset along X axis.
    void set_scroll_offset_x(float offset);

    /// @brief Set scroll offset along Y axis.
    void set_scroll_offset_y(float offset);

    /// @brief Default scroll widget update for smooth scrolling.
    static void on_update(UIWidget widget, float delta);

    /// @brief Default scroll widget rendering.
    static void on_draw(UIWidget widget, ScreenRenderComponent renderer);
};

struct UIScrollStorage
{
    Color bgColor;
};

} // namespace LD