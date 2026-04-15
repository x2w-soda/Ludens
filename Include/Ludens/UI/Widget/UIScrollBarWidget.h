#pragma once

#include <Ludens/UI/UIWidget.h>

namespace LD {

class UIScrollBarData
{
    friend struct UIScrollBarWidgetObj;

public:
    Color bgColor = 0;
    Color barColor = 0xFFFFFFFF;
    UIAxis axis = UI_AXIS_Y;

    inline float get_ratio() const { return mRatio; }
    inline void set_ratio(float ratio) { mRatio = std::clamp(ratio, 0.0f, 1.0f); }

private:
    float mRatio = 0.0f;
    float offsetDst = 0.0f;   // destination value for scrollOffset x
    float offsetSpeed = 0.0f; // animation speed for scrollOffset x
};

struct UIScrollBarWidget : UIWidget
{
};

} // namespace LD