#pragma once

#include <Ludens/Event/Event.h>
#include <Ludens/Header/Color.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/Header/Math/Vec2.h>
#include <cstdint>

struct GLFWwindow;

namespace LD {

struct Event;
struct Bitmap;

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

using WindowID = uint32_t;

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

/// @brief Window registry singleton. The root window is created and destroyed along the registry.
struct WindowRegistry : Handle<struct WindowRegistryObj>
{
    /// @brief Create window registry singleton.
    static WindowRegistry create(const WindowInfo& rootWindowInfo);

    /// @brief Destroy window registry singleton and the root window.
    static void destroy();

    /// @brief Get singleton handle.
    static WindowRegistry get();

    /// @brief Get root window ID.
    WindowID get_root_id();

    /// @brief Get delta time in seconds, uniform across all windows.
    double get_delta_time();

    /// @brief Polls events for all windows.
    void poll_events();

    /// @brief Create a child window.
    WindowID create_window(const WindowInfo& windowInfo, WindowID parentID);

    /// @brief Signal the last frame of the window, closes window after the current frame is completed
    void close_window(WindowID id);

public: // window getters
    /// @brief Get native GLFW handle.
    GLFWwindow* get_window_glfw_handle(WindowID id);

    /// @brief Check if window is still open.
    bool is_window_open(WindowID id);

    /// @brief Check if window is minimized.
    bool is_window_minimized(WindowID id);

    /// @brief Get window size extent, not necessarily in pixels.
    Vec2 get_window_extent(WindowID id);

    /// @brief Get window aspect ratio.
    float get_window_aspect_ratio(WindowID id);

    bool get_window_key(WindowID id, KeyCode key);
    bool get_window_key_up(WindowID id, KeyCode key);
    bool get_window_key_down(WindowID id, KeyCode key);

    bool get_window_mouse(WindowID id, MouseButton button);
    bool get_window_mouse_up(WindowID id, MouseButton button);
    bool get_window_mouse_down(WindowID id, MouseButton button);

    bool get_window_mouse_position(WindowID id, float& x, float& y);
    bool get_window_mouse_motion(WindowID id, float& dx, float& dy);

public: // window setters
    /// @brief Makes cursor visible and behave normally.
    void set_window_cursor_mode_normal(WindowID id);

    /// @brief Hides and grabs the cursor.
    void set_window_cursor_mode_disabled(WindowID id);

    /// @brief Hint at the platform window manager to use icon for decoration.
    /// @param iconCount Number of candidates in icons array.
    /// @param icons Candidate bitmaps for icon, good sizes include 16x16, 32x32 and 48x48.
    /// @warning Candidate bitmap format must be BITMAP_FORMAT_RGBA8U.
    void hint_window_icon(WindowID id, int iconCount, Bitmap* icons);

    /// @brief Hint at the platform window manager to use border color
    void hint_window_border_color(WindowID id, Color color);

    /// @brief Hint at the platform window manager to use title bar color
    void hint_window_title_bar_color(WindowID id, Color color);

    /// @brief Hint at the platform window manager to use title bar text color
    void hint_window_title_bar_text_color(WindowID id, Color color);

    /// @brief Hint at the platform window manager to use title text
    /// @param cstr null terminated C string
    void hint_window_title_bar_text(WindowID id, const char* cstr);

    void hint_window_cursor_shape(WindowID id, CursorType cursor);
};

} // namespace LD