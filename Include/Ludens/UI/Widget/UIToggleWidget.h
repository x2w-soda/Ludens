#pragma once

#include <Ludens/UI/UIWidget.h>

namespace LD {

struct UIToggleStorage
{
    bool state;
};

typedef void (*UIToggleOnToggle)(UIWidget w, bool state, void* user);

/// @brief UI toggle widget, is either in the "true" or "false" boolean state.
struct UIToggleWidget : UIWidget
{
    UIToggleStorage* get_storage();
    void set_on_toggle(UIToggleOnToggle onToggle);
};

} // namespace LD