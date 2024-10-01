#include "Core/UI/Include/UI.h"
#include "Core/UI/Include/UIWidget.h"
#include "Core/UI/Include/Control/Control.h"
#include "Core/UI/Include/Container/Container.h"

namespace LD
{

#define CASE(TYPE)                                                                                                     \
    case UIType::TYPE:                                                                                                 \
        return "UI" #TYPE;

String UITypeString(UIType type)
{
    switch (type)
    {
        CASE(Window)
        CASE(Scroll)
        CASE(Panel)
        CASE(Label)
        CASE(Button)
        CASE(Texture)
    }

    LD_DEBUG_UNREACHABLE;
}
#undef CASE

float UIText::GetGlyphScale() const
{
    LD_DEBUG_ASSERT(Font);
    return Size / Font->GetTTF()->GetPixelSize();
}

UIWidget::UIWidget(UIType type) : mType(type), mFlags(0), mNext(nullptr), mParent(nullptr), mChild(nullptr), mHasStartup(false)
{
}

UIWidget::~UIWidget()
{
    LD_DEBUG_ASSERT(!mHasStartup);
}

void UIWidget::Startup(const UIWidgetInfo& info)
{
    LD_DEBUG_ASSERT(!mHasStartup);

    Attach(info.Parent);

    mUserCallback = info.Callback;

    UILayoutNode* parentLayout = nullptr;

    if (info.Parent && info.Parent->mType == UIType::Scroll)
        parentLayout = static_cast<UIScroll*>(info.Parent)->GetLayoutRoot();
    else if (info.Parent && mType != UIType::Window)
        parentLayout = &info.Parent->mLayout;

    UILayoutNodeInfo layoutInfo;
    layoutInfo.Parent = parentLayout;
    layoutInfo.Width = info.Width;
    layoutInfo.Height = info.Height;
    layoutInfo.FlexDirection = info.FlexDirection;
    layoutInfo.FlexGrow = info.FlexGrow;
    mLayout.Startup(layoutInfo);

    mHasStartup = true;
}

void UIWidget::Cleanup()
{
    if (!mHasStartup)
        return;

    mHasStartup = false;

    // NOTE: child Detach() modifies the mChild linked list,
    //       hence the while loop instead of for loop iteration
    while (mChild)
    {
        CleanupRecursive(mChild);
    }

    mLayout.Cleanup();

    Detach();
}

void UIWidget::CalculateLayout()
{
    mLayout.CalculateLayout();
}

UIType UIWidget::GetType() const
{
    return mType;
}

void UIWidget::SetPadding(float padding)
{
    mLayout.SetPadding(padding);
}

void UIWidget::SetPadding(float padding, UIEdge edge)
{
    mLayout.SetPadding(padding, edge);
}

void UIWidget::SetMargin(float margin)
{
    mLayout.SetMargin(margin);
}

void UIWidget::SetMargin(float margin, UIEdge edge)
{
    mLayout.SetMargin(margin, edge);
}

Vec2 UIWidget::GetPos() const
{
    Vec2 pos;

    mLayout.GetPos(pos);

    return pos;
}

Rect2D UIWidget::GetRect() const
{
    Vec2 pos;
    Vec2 size;

    mLayout.GetSize(size);
    mLayout.GetPos(pos);

    return { pos.x, pos.y, size.x, size.y };
}

void UIWidget::OnEnter()
{
    LD_DEBUG_ASSERT(mFlags & IS_HOVERABLE_BIT);

    UIContext* ctx = mWindow->GetContext();

    mFlags |= IS_HOVERED_BIT;

    // TODO: cursor hint

    if (mLibCallback.OnEnter)
        mLibCallback.OnEnter(ctx, this);

    if (mUserCallback.OnEnter)
        mUserCallback.OnEnter(ctx, this);
}

void UIWidget::OnLeave()
{
    LD_DEBUG_ASSERT(mFlags & IS_HOVERABLE_BIT);

    UIContext* ctx = mWindow->GetContext();

    mFlags &= ~IS_HOVERED_BIT;

    // TODO: cursor hint

    if (mLibCallback.OnLeave)
        mLibCallback.OnLeave(ctx, this);

    if (mUserCallback.OnLeave)
        mUserCallback.OnLeave(ctx, this);
}

void UIWidget::OnPress()
{
    LD_DEBUG_ASSERT(mFlags & IS_PRESSABLE_BIT);

    UIContext* ctx = mWindow->GetContext();

    mFlags |= IS_PRESSED_BIT;

    if (mLibCallback.OnPress)
        mLibCallback.OnPress(ctx, this);

    if (mUserCallback.OnPress)
        mUserCallback.OnPress(ctx, this);

    // TODO: widget drag
}

void UIWidget::OnScroll(float dx, float dy)
{
    LD_DEBUG_ASSERT(mFlags & IS_SCROLLABLE_BIT);

    UIContext* ctx = mWindow->GetContext();

    if (mLibCallback.OnScroll)
        mLibCallback.OnScroll(ctx, this, dx, dy);

    if (mUserCallback.OnScroll)
        mUserCallback.OnScroll(ctx, this, dx, dy);
}

void UIWidget::OnRelease()
{
    LD_DEBUG_ASSERT(mFlags & IS_PRESSABLE_BIT);

    UIContext* ctx = mWindow->GetContext();

    mFlags &= ~IS_PRESSED_BIT;

    if (mLibCallback.OnRelease)
        mLibCallback.OnRelease(ctx, this);

    if (mUserCallback.OnRelease)
        mUserCallback.OnRelease(ctx, this);
}

void UIWidget::CleanupRecursive(UIWidget* widget)
{
    UIType type = widget->GetType();
    switch (type)
    {
    case UIType::Panel:
        static_cast<UIPanel*>(widget)->Cleanup();
        break;
    case UIType::Label:
        static_cast<UILabel*>(widget)->Cleanup();
        break;
    case UIType::Button:
        static_cast<UIButton*>(widget)->Cleanup();
        break;
    case UIType::Texture:
        static_cast<UITexture*>(widget)->Cleanup();
        break;
    case UIType::Scroll:
        static_cast<UIScroll*>(widget)->Cleanup();
        break;
    default:
        LD_DEBUG_UNREACHABLE;
    }
}

void UIWidget::Attach(UIWidget* parent)
{
    if (!parent) // window widget
    {
        LD_DEBUG_ASSERT(mType == UIType::Window);

        mWindow = (UIWindow*)this;
        mParent = mNext = mChild = nullptr;
        mContainer = (UIContainerWidget*)this;
        return;
    }

    // attach to parent and window
    mWindow = parent->mWindow;
    mParent = parent;
    mNext = parent->mChild;
    parent->mChild = this;

    // attach to container
    bool parentIsContainer = (parent->GetFlags() & UIWidget::IS_CONTAINER_BIT);
    mContainer = parentIsContainer ? (UIContainerWidget*)parent : parent->mContainer;
    mContainer->AddToContainer(this);
}

void UIWidget::Detach()
{
    if (!mParent) // window widget
    {
        mWindow = nullptr;
        return;
    }

    // detach from container
    mContainer->RemoveFromContainer(this);
    mContainer = nullptr;

    // detach from parent and window
    UIWidget** w;
    for (w = &mParent->mChild; *w && *w != this; w = &((*w)->mNext))
        ;

    LD_DEBUG_ASSERT(*w != nullptr);

    *w = (*w)->mNext;
    mParent = nullptr;
    mWindow = nullptr;
}

UIContainerWidget::UIContainerWidget(UIType type) : UIWidget(type)
{
    mFlags |= UIWidget::IS_CONTAINER_BIT;
}

void UIContainerWidget::AddToContainer(UIWidget* widget)
{
    mWidgetStack.PushTop(widget);
}

void UIContainerWidget::RemoveFromContainer(UIWidget* widget)
{
    mWidgetStack.Remove(widget);
}

void UIContainerWidget::Debug(String& str)
{
    Debug(str, 0);
}

void UIContainerWidget::Debug(String& str, int indent)
{
    for (int i = 0; i < indent; i++)
        str += " ";

    str += UITypeString(mType);
    str += "\n";

    indent += 2;

    for (UIWidget* widget : GetWidgets())
    {
        if (widget->GetFlags() & UIWidget::IS_CONTAINER_BIT)
        {
           static_cast<UIContainerWidget*>(widget)->Debug(str, indent);
        }
        else
        {
            for (int i = 0; i < indent; i++)
                str += " ";

            str += UITypeString(widget->GetType());
            str += "\n";
        }
    }
}

UIWidget* UIContainerWidget::GetTopWidget(const Vec2& pos, bool (*filter)(UIWidget*))
{
    for (int i = (int)mWidgetStack.Size() - 1; i >= 0; i--)
    {
        UIWidget* widget = mWidgetStack[i];
        Rect2D rect = AdjustedRect(widget->GetRect());
        u32 flags = widget->GetFlags();

        if (!rect.Contains(pos))
            continue;

        // recursive container search
        if (flags & UIWidget::IS_CONTAINER_BIT)
        {
            Vec2 localPos = pos - rect.Min();
            return static_cast<UIContainerWidget*>(widget)->GetTopWidget(localPos, filter);
        }

        // apply filter on non-container leaf widget
        if (filter(widget))
            return widget;
        
    }

    // apply filter on self
    return filter(this) ? this : nullptr;
}

} // namespace LD