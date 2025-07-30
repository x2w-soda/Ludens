#pragma once

#include <Ludens/Header/Color.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <cstdint>

namespace LD {

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

struct Event;

struct ApplicationInfo
{
    const char* name;                                /// application window name
    void* user;                                      /// arbitrary user data
    void (*onEvent)(const Event* event, void* user); /// event callback
    uint32_t width;                                  /// application window width
    uint32_t height;                                 /// application window height
    bool vsync;                                      /// whether vsync is enabled
    Color hintBorderColor;                           /// If not zero, the desired window border color.
    Color hintTitleBarColor;                         /// If not zero, the desired window title bar color.
    Color hintTitleBarTextColor;                     /// If not zero, the desired window title bar text color.
};

/// @brief handle of a windowed application
class Application
{
public:
    Application() = delete;

    /// @brief create application singleton
    /// @param appI application info
    /// @return handle to the singleton application
    static Application create(const ApplicationInfo& appI);

    /// @brief destroy application singleton
    static void destroy();

    /// @brief get singleton handle
    static Application get();

    /// @brief pass an event to the application singleton
    static void on_event(const Event* event);

    /// @brief get window width
    uint32_t width() const;

    /// @brief get window height
    uint32_t height() const;

    /// @brief get window aspect ratio
    float aspect_ratio() const;

    /// @brief check whether the window is currently minimized
    bool is_window_minimized();

    /// @brief chech whether the window is still active
    bool is_window_open();

    /// @brief poll window events
    void poll_events();

    /// @brief get render device
    RDevice get_rdevice();

    /// @brief get time in seconds since the Application is created
    double get_time();

    /// @brief get time in seconds between the current frame and previous frame
    double get_delta_time();

    /// @brief signal the last frame of the application, closes window after the current frame is completed
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

private:
    Application(struct ApplicationObj* obj);

    struct ApplicationObj* mObj;
};

} // namespace LD