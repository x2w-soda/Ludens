#pragma once

#include <Ludens/Header/Handle.h>
#include <cstdint>
#include <string>

namespace LD {

/// @brief Settings for starting up the game runtime.
struct ProjectStartupSettings : Handle<struct ProjectSettingsObj>
{
    uint32_t get_window_width();
    void set_window_width(uint32_t width);

    uint32_t get_window_height();
    void set_window_height(uint32_t height);

    std::string get_window_name();
    void set_window_name(const std::string& name);
};

/// @brief Ground truth data for project-wide settings.
struct ProjectSettings : Handle<struct ProjectSettingsObj>
{
    /// @brief Creates empty project settings with defaults.
    static ProjectSettings create();

    /// @brief Destroy project settings data.
    static void destroy(ProjectSettings settings);

    /// @brief Get interface for startup settings.
    ProjectStartupSettings get_startup_settings();
};

} // namespace LD