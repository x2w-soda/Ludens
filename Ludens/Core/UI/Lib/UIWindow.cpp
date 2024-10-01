#include "Core/UI/Include/UI.h"
#include "Core/UI/Include/UIWidget.h"

namespace LD
{

UIWindow::UIWindow() : UIContainerWidget(UIType::Window)
{
}

UIWindow::~UIWindow()
{
    LD_DEBUG_ASSERT(mContext == nullptr);
}

void UIWindow::Startup(const UIWindowInfo& info)
{
    UITheme* theme = info.Context->GetTheme();

    mContext = info.Context;
    mDebugName = info.DebugName;
    mRect = info.Rect;

    float border, padding;
    theme->GetBackgroundColor(mColor);
    theme->GetWindowBorder(border);
    theme->GetWindowPadding(padding);

    LD_DEBUG_ASSERT(mContext);
    LD_DEBUG_ASSERT(mRect.w > 0 && mRect.h > 0);

    Attach(info.Parent);

    UIWidgetInfo widgetInfo{};
    widgetInfo.Width = mRect.w;
    widgetInfo.Height = mRect.h;
    widgetInfo.Parent = nullptr;
    widgetInfo.FlexDirection = UIFlexDirection::Column;
    UIWidget::Startup(widgetInfo);

    mLayout.SetBorder(border);
    mLayout.SetPadding(padding);
}

void UIWindow::Cleanup()
{
    UIWidget::Cleanup();

    Detach();
    mContext = nullptr;
}

Vec2 UIWindow::GetWindowPos() const
{
    return { mRect.x, mRect.y };
}

void UIWindow::SetWindowPos(const Vec2& pos)
{
    mRect.x = pos.x;
    mRect.y = pos.y;
}

Vec2 UIWindow::GetWindowSize() const
{
    return { mRect.w, mRect.h };
}

float UIWindow::GetBorder() const
{
    float border;
    mLayout.GetBorder(border);

    return border;
}

Rect2D UIWindow::GetWindowRect() const
{
    return mRect;
}

void UIWindow::SetColor(const Vec4& color)
{
    mColor = color;
}

Vec4 UIWindow::GetColor() const
{
    return mColor;
}

void UIWindow::Raise()
{
    LD_DEBUG_ASSERT(mContext);

    mContext->RaiseWindow(this);
}

void UIWindow::InputMouseScroll(const Vec2& pos, float dx, float dy)
{
    auto filter = [](UIWidget* widget) -> bool { return widget->GetFlags() & UIWidget::IS_SCROLLABLE_BIT; };
    UIWidget* widget = GetTopWidget(pos, filter);

    if (!widget)
        return;

    widget->OnScroll(dx, dy);
}

void UIWindow::InputMouseButtonPressed(const Vec2& pos, MouseButton button)
{
    auto filter = [](UIWidget* widget) -> bool { return widget->GetFlags() & UIWidget::IS_PRESSABLE_BIT; };
    UIWidget* widget = GetTopWidget(pos, filter);

    if (!widget)
        return;

    widget->OnPress();
}

void UIWindow::InputMouseButtonReleased(const Vec2& pos, MouseButton button)
{
    auto filter = [](UIWidget* widget) -> bool { return widget->GetFlags() & UIWidget::IS_PRESSABLE_BIT; };
    UIWidget* widget = GetTopWidget(pos, filter);

    if (!widget)
        return;

    widget->OnRelease();
}

void UIWindow::InputKeyPress(KeyCode key)
{
    // TODO:
}

void UIWindow::InputKeyRelease(KeyCode key)
{
    // TODO:
}

void UIWindow::Attach(UIWindow* parent)
{
    mContext->mWindowStack.PushTop(this);

    if (!parent) // context root window
    {
        mParent = mNext = mChild = nullptr;
        return;
    }

    mParent = parent;
    mNext = parent->mChild;
    parent->mChild = this;
}

void UIWindow::Detach()
{
    mContext->mWindowStack.Remove(this);

    if (!mParent) // context root window
        return;

    UIWindow** w;
    for (w = &mParent->mChild; *w && *w != this; w = &((*w)->mNext))
        ;

    LD_DEBUG_ASSERT(*w != nullptr);
    *w = (*w)->mNext;
    mParent = nullptr;
}

} // namespace LD