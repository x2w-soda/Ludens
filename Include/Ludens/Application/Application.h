#pragma once

#include <cstdint>
#include <Ludens/RenderBackend/RBackend.h>

namespace LD {

struct ApplicationInfo
{
    const char* name;
    uint32_t width;
    uint32_t height;
};

/// @brief A windowed application
class Application
{
public:
    Application() = delete;
    Application(const ApplicationInfo& appI);
    Application(const Application&) = delete;

    Application& operator=(const Application&) = delete;

    ~Application();

    /// @brief get window width
    uint32_t width() const;

    /// @brief get window height
    uint32_t height() const;

    /// @brief get window aspect ratio
    float aspect_ratio() const;

    bool is_window_open();
    void poll_events();
    RDevice get_rdevice();

    /// @brief get time in seconds since the Application is created
    double get_time();

    void set_cursor_mode_normal();
    void set_cursor_mode_disabled();

private:
    struct Window* mWindow;
};

} // namespace LD