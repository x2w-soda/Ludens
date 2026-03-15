#pragma once

#include <Ludens/UI/UIWidget.h>

namespace LD {

struct UIPanelStorage
{
    Color color = 0;
};

struct UIPanelWidget : UIWidget
{
    UIPanelStorage* get_storage();

    /// @brief Default panel widget rendering.
    static void on_draw(UIWidget widget, ScreenRenderComponent renderer);
};

} // namespace LD