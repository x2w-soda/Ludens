#pragma once

#include <string>
#include <unordered_set>
#include "Core/OS/Include/Time.h"
#include "Core/Math/Include/Rect2D.h"
#include "Core/Math/Include/Vec2.h"
#include "Core/Math/Include/Vec4.h"
#include "Core/DSA/Include/Optional.h"
#include "Core/DSA/Include/Vector.h"
#include "Core/DSA/Include/View.h"
#include "Core/DSA/Include/String.h"
#include "Core/Application/Include/Input.h"
#include "Core/Media/Include/Font.h"
#include "Core/UI/Include/UILayout.h"
#include "Core/UI/Include/UILogicStack.h"
#include "Core/UI/Include/UIWidget.h"
#include "Core/UI/Include/UITheme.h"

namespace LD
{

class UIContext;
class UIWindow;
class UIWidget;
class UIPanel;
class UIScroll;

struct UIFontInfo
{
    Ref<FontTTF> TTF;
    Ref<FontGlyphTable> GlyphTable;
};

/// font data required by the UI framework
class UIFont
{
public:
    UIFont();
    UIFont(const UIFont&) = delete;
    ~UIFont();

    void Startup(const UIFontInfo& info);
    void Cleanup();

    Ref<FontTTF> GetTTF();
    Ref<FontGlyphTable> GetGlyphTable();

    /// @brief calculate width and height from text when rendered on a single line
    /// @param text the text to calculate size from
    /// @param ratio the desired display size divided by actual font/glyph size
    /// @param size the width and height that the text occupies
    /// @param glyphs a pointer to an array of glyphs, one glyph per character in text
    /// @return true of all glyphs in text are found, false otherwise
    bool DeriveTextSize(const UIString& text, float ratio, Vec2& size, FontGlyphExt* glyphs);

    /// @brief calculate width and height from text when rendered on a single line
    /// @param text the text to calculate size from
    /// @param ratio the desired display size divided by actual font/glyph size
    /// @param width the limiting width per line of text
    /// @param height the resulting height, likely an integer multiple of font line space
    /// @param glyphs a pointer to an array of glyphs, one glyph per character in text
    /// @return true of all glyphs in text are found, false otherwise
    bool DeriveTextSizeLimitWidth(const UIString& text, float ratio, float width, float& height, FontGlyphExt* glyphs);

private:
    Ref<FontTTF> mTTF;
    Ref<FontGlyphTable> mGlyphTable;
};

struct UIText
{
    UIFont* Font = nullptr;
    UIString Content;
    float Size;

    /// get the ratio of displayed pixel size to the actual glyph size
    float GetGlyphScale() const;
};

struct UIWindowInfo
{
    UIContext* Context;

    UIWindow* Parent;

    /// debug name, not displayed as title
    UIString DebugName;

    /// position and size of the window. the position is in root coordinates.
    Rect2D Rect;
};

class UIWindow : public UIContainerWidget
{
public:
    UIWindow();
    UIWindow(const UIWindow&) = delete;
    ~UIWindow();

    UIWindow& operator=(const UIWindow&) = delete;

    void Startup(const UIWindowInfo& info);
    void Cleanup();

    UIContext* GetContext()
    {
        return mContext;
    }

    /// get first child window
    UIWindow* GetWindowChild() const
    {
        return mChild;
    }

    /// get next sibling window
    UIWindow* GetWindowNext() const
    {
        return mNext;
    }

    /// get the window's position in root coordinates
    Vec2 GetWindowPos() const;

    /// set the window's position in root coordinates
    void SetWindowPos(const Vec2& pos);

    /// get width and height of the window
    Vec2 GetWindowSize() const;

    /// get window border width
    float GetBorder() const;

    /// get window root position and size
    Rect2D GetWindowRect() const;

    /// set window background color
    void SetColor(const Vec4& color);

    /// get window background color
    Vec4 GetColor() const;

    /// raise the window to the top
    void Raise();

    // by default, the context forwards input events to destination window,
    // but the user can also directly inject input into a specific window.

    void InputMouseScroll(const Vec2& pos, float dx, float dy);
    void InputMouseButtonPressed(const Vec2& pos, MouseButton button);
    void InputMouseButtonReleased(const Vec2& pos, MouseButton button);
    void InputKeyPress(KeyCode key);
    void InputKeyRelease(KeyCode key);

private:
    void Attach(UIWindow* parent);
    void Detach();

    Rect2D mRect; // position and size relative to root window
    Vec4 mColor;
    UIString mDebugName;
    UIContext* mContext = nullptr;
    UIWindow* mParent;      // parent window
    UIWindow* mChild;       // first child window
    UIWindow* mNext;        // next sibling window
};

struct UIContextInfo
{
    float Width;
    float Height;
};

/// UI elements live within a context, and can not interact with UI elements
/// residing in another context.
class UIContext
{
    friend class UIWindow;

public:
    UIContext() = default;
    UIContext(const UIContext&) = delete;
    ~UIContext();

    UIContext& operator=(const UIContext&) = delete;

    void Startup(const UIContextInfo& info);
    void Cleanup();

    UITheme* GetTheme();

    UIWindow* GetRoot();

    /// get the width and height of this context, which
    /// is defined by the size of the root window.
    Vec2 GetSize();

    /// get a view to all the windows in the context, from bottom to top.
    /// the view is invalidated whenever window ordering changes.
    View<UIWindow*> GetWindows()
    {
        return mWindowStack.GetView();
    }

    /// get the widget hovered by mouse cursor, or nullptr
    UIWidget* GetHoverWidget()
    {
        return mHoverWidget;
    }

    void BeginFrame(DeltaTime dt);
    void EndFrame();

    /// raise window to the top
    void RaiseWindow(UIWindow* window);

    // UI context still requires a driver that continuosly updates the input state,
    // user can freeze or save some CPU by not updating contexts that are inactive.

    void InputMousePosition(Vec2 pos);
    void InputMouseScroll(float& deltaX, float& deltaY);
    void InputMouseButtonPressed(MouseButton button);
    void InputMouseButtonReleased(MouseButton button);
    void InputKeyPress(KeyCode key);
    void InputKeyRelease(KeyCode key);

    void AddLayoutRoot(UILayoutNode* root);
    void RemoveLayoutRoot(UILayoutNode* root);

private:
    UIWindow* GetTopWindow(const Vec2& pos);

    std::unordered_set<UILayoutNode*> mLayoutRoots;
    UILogicStack<UIWindow> mWindowStack; // window stack, one per context
    UIWindow mRoot;                      // root window is provided by context
    UIWindow* mFocus;                    // window receiving key input
    UIWindow* mPress;                    // window last pressed and not released by mouse button
    UIWindow* mHoverWindow;              // window last entered and not leaved by mouse cursor
    UIWidget* mHoverWidget;              // widget last entered and not leaved by mouse cursor
    UIWidget* mDragWidget;               // widget pressed and dragged by mouse cursor
    UIWidget* mLastAttach;               // last widget that is attached to a parent
    UIWidget* mLastDetach;               // last widget that is detached from a parent
    UIWidget* mTooltip;                  // the tooltip widget, rendered beside mouse cursor
    UITheme* mTheme;                     // the current active theme
    Vec2 mMousePos;                      // mouse cursor position in the viewport
    Vec2 mAnchorPos;                     // anchor position, transforms local position to viewport position
    bool mIsWithinFrame = false;
    bool mIsWithinDraw = false;
};

} // namespace LD
