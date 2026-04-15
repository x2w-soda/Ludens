#pragma once

#include <Ludens/UI/UIWidget.h>

namespace LD {

class UIScrollBarData
{
    friend struct UIScrollBarWidgetObj;

public:
    Color bgColor = 0;
    Color barColor = 0xFFFFFFFF;
    UIAxis axis = UI_AXIS_Y;    // vertical or horizontal scrollbar
    float scrollExtent = 0.0f;  // scroll container extent along the axis
    float contentExtent = 0.0f; // content extent along the axis

    inline float get_ratio() const { return mRatio; }
    inline void set_ratio(float ratio) { mRatio = std::clamp(ratio, 0.0f, 1.0f); }

private:
    Rect mBarRect = {};  // computed from scroll extent, content extent, and scroll ratio.
    float mRatio = 0.0f; // scroll bar normalized ratio
};

struct UIScrollBarWidget : UIWidget
{
};

} // namespace LD