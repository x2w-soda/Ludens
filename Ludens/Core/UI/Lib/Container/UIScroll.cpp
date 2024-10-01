#include "Core/UI/Include/UI.h"
#include "Core/UI/Include/Container/UIScroll.h"

namespace LD
{

UIScroll::UIScroll() : UIContainerWidget(UIType::Scroll), mScroll(0.0f)
{
    mFlags |= UIWidget::IS_SCROLLABLE_BIT;
}

UIScroll::~UIScroll()
{
}

void UIScroll::Startup(const UIScrollInfo& info)
{
    UIWidgetInfo widgetI;
    widgetI.Parent = info.Parent;
    widgetI.FlexGrow = 1.0f;
    UIWidget::Startup(widgetI);

    mLibCallback.OnScroll = &UIScroll::OnScroll;

    // scroll container uses two layout nodes, the UIWidget::mLayout node is only
    // used to calculate visible scroll dimension. Another layout node is used
    // as the parent for widgets in the scroll container
    UIContext* ctx = mWindow->GetContext();
    UILayoutNodeInfo layoutI;
    layoutI.Parent = nullptr;
    layoutI.FlexGrow = 1.0f;
    layoutI.FlexDirection = UIFlexDirection::Column; // TODO: flex row and horizontal scroll

    mLayoutRoot.Startup(layoutI);
    ctx->AddLayoutRoot(&mLayoutRoot);
}

void UIScroll::Cleanup()
{
    UIContext* ctx = mWindow->GetContext();
    ctx->RemoveLayoutRoot(&mLayoutRoot);
    mLayoutRoot.Cleanup();

    UIWidget::Cleanup();
}

float UIScroll::GetScroll() const
{
    return mScroll;
}

void UIScroll::SetScroll(float value)
{
    mScroll = std::max(0.0f, value);
}

UILayoutNode* UIScroll::GetLayoutRoot()
{
    return &mLayoutRoot;
}

Rect2D UIScroll::AdjustedRect(const Rect2D& rect)
{
    // take scroll into account
    Rect2D scrolled = rect;
    scrolled.y -= mScroll;

    return scrolled;
}

void UIScroll::OnScroll(UIContext* ctx, UIWidget* widget, float dx, float dy)
{
    // TODO: animate widget properties for smooth scrolling over time
    UIScroll* scroll = (UIScroll*)widget;

    float sc = scroll->GetScroll();
    scroll->SetScroll(sc - dy * 3.0f);
}

} // namespace LD