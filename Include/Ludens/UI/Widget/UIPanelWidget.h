#pragma once

#include <Ludens/UI/UIWidget.h>

namespace LD {

class UIPanelData
{
    friend struct UIPanelWidgetObj;

public:
    Color color = 0;     // if not zero, the backdrop panel color
    float radius = 0.0f; // border radius
};

struct UIPanelWidget : UIWidget
{
};

} // namespace LD