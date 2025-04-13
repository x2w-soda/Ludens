#pragma once

#include <cstdint>

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

    /// @brief Application entry point, returns after Window is closed
    void Run();

private:
    struct Window* mWindow;
};

} // namespace LD