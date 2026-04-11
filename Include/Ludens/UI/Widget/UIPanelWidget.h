#pragma once

#include <Ludens/UI/UIWidget.h>

namespace LD {

struct UIPanelStorage
{
    Color color = 0;     // if not zero, the backdrop panel color
    float radius = 0.0f; // border radius
};

struct UIPanelWidget : UIWidget
{
    UIPanelStorage* get_storage();
    void set_storage(UIPanelStorage* storage);
};

} // namespace LD