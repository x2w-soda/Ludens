#pragma once

#include <Ludens/UI/UIAnimation.h>
#include <Ludens/UI/UIWidget.h>

namespace LD {

typedef void (*UIToggleOnToggle)(UIWidget w, bool state, void* user);

class UIToggleData
{
    friend struct UIToggleWidgetObj;

public:
    bool state;
    UIToggleOnToggle onToggle = nullptr;

private:
    UIAnimation<QuadraticInterpolation> mAnim;
};

/// @brief UI toggle widget, is either in the "true" or "false" boolean state.
struct UIToggleWidget : UIWidget
{
    void set_on_toggle(UIToggleOnToggle onToggle);
};

} // namespace LD