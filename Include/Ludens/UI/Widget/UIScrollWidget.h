#pragma once

#include <Ludens/UI/UIWidget.h>

namespace LD {

class UIScrollData
{
    friend struct UIScrollWidgetObj;

public:
    Color bgColor;

    inline Vec2 get_min_scroll_offset() { return mMinScrollOffset; }

private:
    Vec2 mOffsetDst = {};       // destination value for scrollOffset
    Vec2 mOffsetSpeed = {};     // animation speed for scrollOffset
    Vec2 mMinScrollOffset = {}; // used to clamp scroll offset
    bool mSnapToDst = false;
};

struct UIScrollWidget : UIWidget
{
    /// @brief Set scroll offset along X axis.
    void set_scroll_offset_x(float offset);
    void set_scroll_offset_x_normalized(float ratio);
    float get_scroll_offset_x_normalized();

    /// @brief Set scroll offset along Y axis.
    void set_scroll_offset_y(float offset);
    void set_scroll_offset_y_normalized(float ratio);
    float get_scroll_offset_y_normalized();
};

} // namespace LD