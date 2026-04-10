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
    void set_storage(UIPanelStorage* storage);
};

} // namespace LD