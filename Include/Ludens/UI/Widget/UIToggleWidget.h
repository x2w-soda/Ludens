#pragma once

#include <Ludens/UI/UIWidget.h>

namespace LD {

typedef void (*UIToggleOnToggle)(UIWidget w, bool state, void* user);

/// @brief UI toggle widget, is either in the "true" or "false" boolean state.
struct UIToggleWidget : UIWidget
{
    void set_on_toggle(UIToggleOnToggle onToggle);

    /// @brief Default image widget rendering.
    static void on_draw(UIWidget widget, ScreenRenderComponent renderer);
};

struct UIToggleStorage
{
    bool state;
};

} // namespace LD