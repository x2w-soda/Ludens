#include "Core/UI/Include/Control/UIPanel.h"

namespace LD
{

UIPanel::UIPanel() : UIWidget(UIType::Panel)
{
}

UIPanel::~UIPanel()
{
    UIWidget::Cleanup();
}

void UIPanel::Startup(const UIPanelInfo& info)
{
    UIWidget::Startup(info.Widget);

    mColor = info.Color;
}

void UIPanel::Cleanup()
{
    UIWidget::Cleanup();
}

Vec4 UIPanel::GetColor() const
{
    return mColor;
}

void UIPanel::SetColor(const Vec4& color)
{
    mColor = color;
}

} // namespace LD