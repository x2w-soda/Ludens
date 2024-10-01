#include "Core/UI/Include/UI.h"
#include "Core/UI/Include/UIWidget.h"

// TODO: remove
#include <iostream>

namespace LD {

///
/// Window and Context
///

UIWindow::UIWindow() : UIWidget(UIType::Window)
{
}

UIWindow::~UIWindow()
{
    LD_DEBUG_ASSERT(mContext == nullptr);
}

void UIWindow::Startup(const UIWindowInfo& info, UIContext* context)
{
    mContext = context;
    mRect = info.Rect;

    Attach(info.Parent);

    UIWidgetInfo widgetInfo{};
    widgetInfo.Width = mRect.w;
    widgetInfo.Height = mRect.h;
    widgetInfo.Parent = nullptr;
    UIWidget::Startup(widgetInfo);
    // TODO: Set flex direction row vs column for root node
}

void UIWindow::Cleanup()
{
    UIWidget::Cleanup();

    Detach();
    mContext = nullptr;
}

Vec2 UIWindow::GetPosition() const
{
    return mRect.Min();
}

Vec2 UIWindow::GetSize() const
{
    return { mRect.w, mRect.h };
}

Rect2D UIWindow::GetRect() const
{
    return mRect;
}



void UIWindow::InputMouseButtonPressed(MouseButton button, const Vec2& pos)
{
    UIWidget* widget = GetTopWidget(pos);

    if (!widget)
        return;

    widget->OnPress();
}

void UIWindow::InputMouseButtonReleased(MouseButton button, const Vec2& pos)
{
    UIWidget* widget = GetTopWidget(pos);
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
    mContext->mStack.Insert(this);

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
    mContext->mStack.Remove(this);

    if (!mParent) // context root window
        return;

    UIWindow** w;
    for (w = &mParent->mChild; *w && *w != this; w = &((*w)->mNext))
        ;

    LD_DEBUG_ASSERT(*w != nullptr);
    *w = (*w)->mNext;
    mParent = nullptr;
}

UIWidget* UIWindow::GetTopWidget(const Vec2& pos)
{
    for (int i = (int)mStack.Size() - 1; i >= 0; i--)
    {
        UIWidget* widget = mStack[i];
        Rect2D rect = widget->GetRect();

        if (rect.Contains(pos))
            return widget;
    }

    return nullptr;
}

UIContext::~UIContext()
{
    LD_DEBUG_ASSERT(mStack.Size() == 0);
}

void UIContext::Startup(const UIContextInfo& info)
{
    UIWindowInfo rootInfo;
    rootInfo.Parent = nullptr;
    rootInfo.Rect = { 0.0f, 0.0f, info.Width, info.Height };
    mRoot.Startup(rootInfo, this);
}

void UIContext::Cleanup()
{
    mRoot.Cleanup();
}

UIWindow* UIContext::GetRoot()
{
    return &mRoot;
}

void UIContext::BeginFrame(DeltaTime dt)
{
    mIsWithinFrame = true;

    mRoot.CalculateLayout();

    for (int i = 0; i < (int)mStack.Size(); i++)
    {
        UIWindow* window = mStack[i];
        window->CalculateLayout();
    }
}

void UIContext::EndFrame()
{
    mIsWithinFrame = false;
}

void UIContext::InputMousePosition(Vec2 pos)
{
    mMousePos = pos;

    // TODO: hot code path, widget hover
}

void UIContext::InputMouseScroll(float& deltaX, float& deltaY)
{
    // TODO: focus widget takes the scroll
}

void UIContext::InputMouseButtonPressed(MouseButton button)
{
    for (int i = (int)mStack.Size() - 1; i >= 0; i--)
    {
        UIWindow* window = mStack[i];
        Rect2D rect = window->GetRect();

        if (rect.Contains(mMousePos))
        {
            Vec2 windowPos = window->GetPosition();
            window->InputMouseButtonPressed(button, mMousePos - windowPos);
            break;
        }
    }
}

void UIContext::InputMouseButtonReleased(MouseButton button)
{
    for (int i = (int)mStack.Size() - 1; i >= 0; i--)
    {
        UIWindow* window = mStack[i];
        Rect2D rect = window->GetRect();

        if (rect.Contains(mMousePos))
        {
            Vec2 windowPos = window->GetPosition();
            window->InputMouseButtonReleased(button, mMousePos - windowPos);
            break;
        }
    }
}

void UIContext::InputKeyPress(KeyCode key)
{
}

void UIContext::InputKeyRelease(KeyCode key)
{
}

} // namespace LD
