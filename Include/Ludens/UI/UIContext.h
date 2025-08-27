#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/Header/KeyCode.h>
#include <Ludens/Header/Math/Rect.h>
#include <Ludens/Header/Math/Vec2.h>
#include <Ludens/UI/UITheme.h>
#include <Ludens/UI/UIWindow.h>
#include <cstdint>
#include <vector>

namespace LD {

struct UIContextInfo
{
    FontAtlas fontAtlas;   /// default font atlas used to render text
    RImage fontAtlasImage; /// font atlas image handle
    UITheme theme;         /// the UI theme to use for widgets in this context
};

/// @brief A UI context is a host for UI elements to be placed on an imaginary 2D grid.
///        UI elements do not communicate across contexts.
struct UIContext : Handle<struct UIContextObj>
{
    /// @brief create a UI context.
    /// @return a UI context handle
    static UIContext create(const UIContextInfo& info);

    /// @brief destroy a UI context.
    static void destroy(UIContext ctx);

    /// @brief update UI context
    /// @param delta delta time in seconds
    void update(float delta);

    /// @brief update mouse cursor position in context
    void input_mouse_position(const Vec2& pos);

    /// @brief notify that a mouse button has been pressed
    void input_mouse_down(MouseButton btn);

    /// @brief notify that a mouse button has been released
    void input_mouse_up(MouseButton btn);

    void input_key_down(KeyCode key);

    void input_key_up(KeyCode key);

    /// @brief perform layout on all widgets across all windows
    void layout();

    /// @brief add a window to the context
    UIWindow add_window(const UILayoutInfo& layoutI, const UIWindowInfo& windowI, void* user);

    /// @brief get window handles
    /// @param windows outputs windows inside the context
    void get_windows(std::vector<UIWindow>& windows);

    /// @brief Get current UI theme, shared by all widgets in this context.
    UITheme get_theme();
};

} // namespace LD