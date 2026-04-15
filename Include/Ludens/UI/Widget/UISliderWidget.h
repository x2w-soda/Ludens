#pragma once

#include <Ludens/UI/UIWidget.h>

namespace LD {

class UISliderData
{
    friend struct UISliderWidgetObj;

public:
    float min = 0.0f; /// slider minimum value
    float max = 1.0f; /// slider maximum value

private:
    Vec2 dragStart;
    float mRatio = 1.0f; /// normalized slider ratio
};

/// @brief UI slider widget
struct UISliderWidget : UIWidget
{
    /// @brief get slider value
    float get_value();
};

} // namespace LD