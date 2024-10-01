#pragma once

#include "Core/UI/Include/UIWidget.h"

namespace LD {

struct UIPanelInfo
{
    UIWidgetInfo Widget;
    Vec4 Color;
};

class UIPanel : public UIWidget
{
public:
    UIPanel();
    UIPanel(const UIPanel&) = delete;
    ~UIPanel();

    UIPanel& operator=(const UIPanel&) = delete;

    void Startup(const UIPanelInfo& info);
    void Cleanup();

    Vec4 GetColor() const;
    void SetColor(const Vec4& color);

private:
    Vec4 mColor;
};

} // namespace LD