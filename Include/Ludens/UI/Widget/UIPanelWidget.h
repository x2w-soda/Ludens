#pragma once

#include <Ludens/UI/UIWidget.h>

namespace LD {

struct UIPanelWidget : UIWidget
{
    /// @brief Default panel widget rendering.
    static void on_draw(UIWidget widget, ScreenRenderComponent renderer);
};

struct UIPanelStorage
{
    Color color = 0;
};

} // namespace LD