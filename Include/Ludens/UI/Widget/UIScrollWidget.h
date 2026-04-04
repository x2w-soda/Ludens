#pragma once

#include <Ludens/UI/UIWidget.h>

namespace LD {

struct UIScrollStorage
{
    Color bgColor;
};

struct UIScrollWidget : UIWidget
{
    UIScrollStorage* get_storage();

    /// @brief Set scroll offset along X axis.
    void set_scroll_offset_x(float offset);

    /// @brief Set scroll offset along Y axis.
    void set_scroll_offset_y(float offset);
};

} // namespace LD