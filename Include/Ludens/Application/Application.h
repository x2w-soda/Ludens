#pragma once

#include <Ludens/RenderBackend/RBackend.h>
#include <cstdint>

namespace LD {

struct ApplicationInfo
{
    const char* name;
    uint32_t width;
    uint32_t height;
    bool vsync;
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

    /// @brief get window width
    uint32_t width() const;

    /// @brief get window height
    uint32_t height() const;

    /// @brief get window aspect ratio
    float aspect_ratio() const;

    /// @brief chech whether the window is still active
    bool is_window_open();
    
    /// @brief set window title name
    /// @param cstr null terminated C string
    void set_window_title(const char* cstr);

    /// @brief poll window events
    void poll_events();

    /// @brief get render device
    RDevice get_rdevice();

    /// @brief get time in seconds since the Application is created
    double get_time();

    /// @brief signal the last frame of the application, closes window after the current frame is completed
    void exit();

    void set_cursor_mode_normal();
    void set_cursor_mode_disabled();

private:
    Application(struct ApplicationObj* obj);

    struct ApplicationObj* mObj;
};

} // namespace LD