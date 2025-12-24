#pragma once

#include <Ludens/Header/Color.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/Header/Math/Vec2.h>
#include <cstdint>

struct GLFWwindow;

namespace LD {

struct Event;

enum CursorType
{
    CURSOR_TYPE_DEFAULT = 0, /// Default system cursor state, usually arrow shape
    CURSOR_TYPE_IBEAM,       /// I-Beam cursor shape, hints at text input
    CURSOR_TYPE_CROSSHAIR,   /// Crosshair cursor shape
    CURSOR_TYPE_HAND,        /// Hand cursor shape
    CURSOR_TYPE_HRESIZE,     /// Horizontal resize cursor shape
    CURSOR_TYPE_VRESIZE,     /// Vertical resize cursor shape
    CURSOR_TYPE_ENUM_COUNT,
};

struct WindowInfo
{
    const char* name;                                /// initial window name
    void* user;                                      /// arbitrary user data
    void (*onEvent)(const Event* event, void* user); /// event callback
    uint32_t width;                                  /// initial window width
    uint32_t height;                                 /// initial window height
    Color hintBorderColor;                           /// if not zero, the desired window border color.
    Color hintTitleBarColor;                         /// if not zero, the desired window title bar color.
    Color hintTitleBarTextColor;                     /// if not zero, the desired window title bar text color.
};

/// @brief Handle of a platform specific Window.
struct Window : Handle<struct WindowObj>
{
    /// @brief Create a window.
    /// @param windowI Window creation info
    static Window create(const WindowInfo& windowI);

    /// @brief Destroy a window.
    static void destroy(Window window);

    /// @brief Get main window handle.
    static Window get();

    /// @brief Pass an event to the main window.
    static void on_event(const Event* event);

    /// @brief Get window width.
    uint32_t width() const;

    /// @brief Get window height.
    uint32_t height() const;

    /// @brief Get window extent.
    Vec2 extent() const;

    /// @brief Get window aspect ratio.
    float aspect_ratio() const;

    /// @brief Check whether the window is currently minimized
    bool is_minimized();

    /// @brief Check whether the window is still active
    bool is_open();

    /// @brief Poll window events.
    void poll_events();

    /// @brief Get native GLFWwindow* handle
    GLFWwindow* get_glfw_window();

    /// @brief get time in seconds since the Application is created
    double get_time();

    /// @brief get time in seconds between the current frame and previous frame
    double get_delta_time();

    /// @brief signal the last frame of the window, closes window after the current frame is completed
    void exit();

    void set_cursor_mode_normal();
    void set_cursor_mode_disabled();

    /// @brief Hint at the platform window manager to use border color
    void hint_border_color(Color color);

    /// @brief Hint at the platform window manager to use title bar color
    void hint_title_bar_color(Color color);

    /// @brief Hint at the platform window manager to use title bar text color
    void hint_title_bar_text_color(Color color);

    /// @brief Hint at the platform window manager to use title text
    /// @param cstr null terminated C string
    void hint_title_bar_text(const char* cstr);

    /// @brief Hint at the platform window manager to use cursor shape
    void hint_cursor_shape(CursorType cursor);
};

} // namespace LD