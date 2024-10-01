#pragma once

#include "Core/Math/Include/Rect2D.h"
#include "Core/Math/Include/Vec4.h"
#include "Core/DSA/Include/Vector.h"
#include "Core/DSA/Include/String.h"
#include "Core/Media/Include/Font.h"
#include "Core/OS/Include/Memory.h"
#include "Core/OS/Include/UID.h"
#include "Core/UI/Include/UILayout.h"
#include "Core/UI/Include/UILogicStack.h"

namespace LD
{

class UIWidget;
class UIContainerWidget;
class UIWindow;
class UIContext;
class UIFont;
using UIWidgetFn = void (*)(UIContext*, UIWidget*);

// TODO: will have to switch to Unicode sooner or later
using UIString = String;

enum class UIType
{
    // container widgets
    Window = 0,
    Scroll,

    // control widgets
    Panel,
    Label,
    Button,
    Texture,
};

String UITypeString(UIType);

class UIPanel;
class UILabel;
class UIButton;
class UITexture;

using UIWidgetOnEnter = void (*)(UIContext*, UIWidget*);
using UIWidgetOnLeave = void (*)(UIContext*, UIWidget*);
using UIWidgetOnPress = void (*)(UIContext*, UIWidget*);
using UIWidgetOnScroll = void (*)(UIContext*, UIWidget*, float, float);
using UIWidgetOnRelease = void (*)(UIContext*, UIWidget*);

struct UIWidgetCallback
{
    UIWidgetOnEnter OnEnter = nullptr;
    UIWidgetOnLeave OnLeave = nullptr;
    UIWidgetOnPress OnPress = nullptr;
    UIWidgetOnScroll OnScroll = nullptr;
    UIWidgetOnRelease OnRelease = nullptr;

    void Reset()
    {
        OnEnter = nullptr;
        OnLeave = nullptr;
        OnPress = nullptr;
        OnScroll = nullptr;
        OnRelease = nullptr;
    }
};

struct UIWidgetInfo
{
    UIWidget* Parent = nullptr;
    UIWidgetCallback Callback;
    UIFlexDirection FlexDirection = UIFlexDirection::Column;
    float FlexGrow = 0.0f;
    float Width = 0.0f;
    float Height = 0.0f;
};

class UIWidget
{
    friend class UIContext;

public:
    UIWidget() = delete;
    UIWidget(UIType type);
    UIWidget(const UIWidget&) = delete;
    ~UIWidget();

    UIWidget& operator=(const UIWidget&) = delete;

    void Startup(const UIWidgetInfo& info);
    void Cleanup();

    void CalculateLayout();

    enum Flags
    {
        IS_CONTAINER_BIT = 1 << 0,
        IS_HOVERABLE_BIT = 1 << 1,
        IS_HOVERED_BIT = 1 << 2,
        IS_PRESSABLE_BIT = 1 << 3,
        IS_PRESSED_BIT = 1 << 4,
        IS_SCROLLABLE_BIT = 1 << 5,
    };

    /// get widget type
    UIType GetType() const;

    /// get position relative to parent window
    Vec2 GetPos() const;

    /// get position and size of this widget, position is relative to parent container
    Rect2D GetRect() const;

    /// get widget flags
    inline u32 GetFlags() const
    {
        return mFlags;
    }

    /// get the window that the widget lives in
    inline UIWindow* GetWindow() const
    {
        LD_DEBUG_ASSERT(mWindow);
        return mWindow;
    }

    /// get first child widget
    inline UIWidget* GetChildren() const
    {
        return mChild;
    }

    /// get next sibling widget
    inline UIWidget* GetNext() const
    {
        return mNext;
    }

    /// set padding for all directions
    void SetPadding(float padding);

    /// set padding for one direction
    void SetPadding(float padding, UIEdge edge);

    /// set left and right padding
    inline void SetHPadding(float padding)
    {
        SetPadding(padding, UIEdge::Left);
        SetPadding(padding, UIEdge::Right);
    }

    /// set top and bottom padding
    inline void SetVPadding(float padding)
    {
        SetPadding(padding, UIEdge::Top);
        SetPadding(padding, UIEdge::Bottom);
    }

    /// set margin for all directions
    void SetMargin(float margin);

    /// set padding for one direction
    void SetMargin(float margin, UIEdge edge);

    /// set left and right padding
    inline void SetHMargin(float margin)
    {
        SetMargin(margin, UIEdge::Left);
        SetMargin(margin, UIEdge::Right);
    }

    /// set top and bottom margin
    inline void SetVMargin(float margin)
    {
        SetMargin(margin, UIEdge::Top);
        SetMargin(margin, UIEdge::Bottom);
    }

    UILayoutNode* GetLayout()
    {
        return &mLayout;
    }

    void OnEnter();
    void OnLeave();
    void OnPress();
    void OnScroll(float dx, float dy);
    void OnRelease();

protected:
    u32 mFlags;
    UIType mType;
    UILayoutNode mLayout;
    UIWidgetCallback mUserCallback;
    UIWidgetCallback mLibCallback;
    UIContainerWidget* mContainer; // first ancestor widget that is a container
    UIWindow* mWindow;             // the window this widget lives in
    UIWidget* mParent;             // parent widget
    UIWidget* mChild;              // first child widget
    UIWidget* mNext;               // next sibling widget

private:
    void CleanupRecursive(UIWidget* widget);
    void Attach(UIWidget* parent);
    void Detach();

    bool mHasStartup;
};

class UIContainerWidget : public UIWidget
{
public:
    UIContainerWidget(UIType type);
    virtual ~UIContainerWidget() = default;

    void AddToContainer(UIWidget* widget);
    void RemoveFromContainer(UIWidget* widget);

    /// @brief find the top-most widget in this container
    /// @param pos point relative to container origin (0, 0)
    /// @param filter a predicate function to filter the widgets in container
    /// @return the top-most widget will all filter flags set, or nullptr
    UIWidget* GetTopWidget(const Vec2& pos, bool (*filter)(UIWidget*));

    /// get a view to all the widgets in the container, from bottom to top.
    /// the view is invalidated whenever widget ordering changes.
    View<UIWidget*> GetWidgets()
    {
        return mWidgetStack.GetView();
    }

    void Debug(String& str);

    /// @brief the container may adjust the rect of the contained widgets
    /// @param rect original widget rect relative to this container
    /// @return rect after the container applies offset or scaling
    /// @see UIScroll container, which overrides this function
    virtual Rect2D AdjustedRect(const Rect2D& rect)
    {
        return rect;
    }

protected:
    void Debug(String& str, int indent);

    UILogicStack<UIWidget> mWidgetStack; // widgets in this container
};

} // namespace LD
