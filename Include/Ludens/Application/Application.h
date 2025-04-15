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

    bool is_window_open();
    void poll_events();
    RDevice get_rdevice();

private:
    struct Window* mWindow;
};

} // namespace LD