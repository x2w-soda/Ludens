#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/Header/KeyCode.h>
#include <Ludens/Header/Math/Rect.h>
#include <Ludens/Header/Math/Vec2.h>
#include <cstdint>
#include <vector>

namespace LD {

enum UIAxis
{
    UI_AXIS_X = 0,
    UI_AXIS_Y,
};

enum UISizeType
{
    UI_SIZE_FIT = 0,
    UI_SIZE_GROW,
    UI_SIZE_FIXED,
    UI_SIZE_WRAP_PRIMARY,
    UI_SIZE_WRAP_SECONDARY,
};

struct UIPadding
{
    float left;
    float right;
    float top;
    float bottom;
};

/// @brief wrap sizing callback, given length limit in main axis,
///        user returns the result size on the secondary axis after wrapping.
typedef float (*UIWrapSizeFn)(void* user, float inLimit);

/// @brief wrap limit callback, user outputs minimum extent of the wrappable
///        and the maximum extent if unwrapped.
typedef void (*UIWrapLimitFn)(void* user, float& outMin, float& outMax);

struct UISize
{
    UIWrapSizeFn wrapSizeFn;
    UIWrapLimitFn wrapLimitFn;
    UISizeType type;
    float extent;

    /// @brief determine size to fit children tightly
    static inline UISize fit()
    {
        return {.type = UI_SIZE_FIT};
    }

    /// @brief expand to take up space in container
    static inline UISize grow()
    {
        return {.type = UI_SIZE_GROW};
    }

    /// @brief if text does not fit into main axis, wrap around and grow along the secondary axis
    static inline UISize wrap_primary(UIWrapSizeFn sizeFn, UIWrapLimitFn limitFn)
    {
        return {.wrapSizeFn = sizeFn, .wrapLimitFn = limitFn, .type = UI_SIZE_WRAP_PRIMARY, .extent = 0.0f};
    }

    static inline UISize wrap_secondary()
    {
        return {.type = UI_SIZE_WRAP_SECONDARY};
    }

    /// @brief declare fixed size for this UI node
    static inline UISize fixed(float extent)
    {
        return {.type = UI_SIZE_FIXED, .extent = extent};
    }
};

/// @brief the layout properties of a UI node.
struct UILayoutInfo
{
    UILayoutInfo() = default;

    /// @brief gap between self and children nodes
    UIPadding childPadding;

    /// @brief the gap between child nodes
    float childGap;

    /// @brief which direction to align children
    UIAxis childAxis;

    /// @brief size layout policy along the horizontal axis
    UISize sizeX;

    /// @brief size layout policy along the vertical axis
    UISize sizeY;
};

struct UIElement : Handle<struct UIElementObj>
{
    /// @brief add a child element
    /// @param layoutI the layout of the child element
    /// @param user arbitrary user data
    /// @return the child element handle
    UIElement add_child(const UILayoutInfo& layoutI, void* user);

    /// @brief set on mouse press callback
    void set_on_press(void (*on_press)(void* user, UIElement element, MouseButton btn));

    /// @brief set on mouse release callback
    void set_on_release(void (*on_release)(void* user, UIElement element, MouseButton btn));

    /// @brief set element on mouse enter callback
    void set_on_enter(void (*on_enter)(void* user, UIElement element));

    /// @brief set element on mouse leave callback
    void set_on_leave(void (*on_leave)(void* user, UIElement element));

    /// @brief set element on drag callback
    void set_on_drag(void (*on_drag)(void* user, UIElement element, MouseButton btn, const Vec2& dragPos, bool begin));

    /// @brief get most recent AABB layout
    Rect get_rect() const;

    /// @brief set user data
    void set_user(void* user);

    /// @brief get user data
    void* get_user();

    /// @brief whether the element is under the mouse cursor
    bool is_hovered() const;

    /// @brief whether the element is being pressed and not yet released
    bool is_pressed() const;
};

struct UIWindowInfo
{
    const char* name;
};

struct UIWindow : UIElement
{
    /// @brief hide the window
    void hide();

    /// @brief show the window
    void show();

    /// @brief check whether the window responds to user input and should be drawn
    bool is_hidden();

    /// @brief set window global position
    /// @param pos position in 2D global space
    void set_pos(const Vec2& pos);

    /// @brief set window size to fixed size
    void set_size(const Vec2& size);

    /// @brief get direct child elements of this window
    void get_children(std::vector<UIElement>& children);
};

/// @brief A UI context is a host for UI elements to be placed on an imaginary 2D grid.
///        UI elements do not communicate across contexts.
struct UIContext : Handle<struct UIContextObj>
{
    /// @brief create a UI context.
    /// @return a UI context handle
    static UIContext create();

    /// @brief destroy a UI context.
    static void destroy(UIContext ctx);

    /// @brief update mouse cursor position in context
    void input_mouse_position(const Vec2& pos);

    /// @brief notify that a mouse button has been pressed
    void input_mouse_press(MouseButton btn);

    /// @brief notify that a mouse button has been released
    void input_mouse_release(MouseButton btn);

    /// @brief perform layout on all widgets across all windows
    void layout();

    /// @brief add a window to the context
    UIWindow add_window(const UILayoutInfo& layoutI, const UIWindowInfo& windowI, void* user);

    void get_windows(std::vector<UIWindow>& windows);
};

} // namespace LD