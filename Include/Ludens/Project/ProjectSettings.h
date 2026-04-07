#pragma once

#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/Header/Math/Vec4.h>
#include <Ludens/Serial/SUID.h>

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

    SUID get_default_scene_id();
    void set_default_scene_id(SUID sceneID);
};

/// @brief Settings for default rendering options.
struct ProjectRenderingSettings : Handle<struct ProjectSettingsObj>
{
    Vec4 get_clear_color();
    void set_clear_color(const Vec4& color);

    static Vec4 get_default_clear_color();
};

struct ProjectScreenLayer
{
    SUID id;
    std::string name;
};

/// @brief Settings for 2D screen layers.
struct ProjectScreenLayerSettings : Handle<struct ProjectSettingsObj>
{
    /// @brief Create a new screen layer.
    SUID create_layer(SUIDRegistry idReg, const char* name);

    /// @brief Create screen layer from known ID.
    /// @return True on success.
    bool create_layer(SUIDRegistry idReg, SUID id, const char* name);

    /// @brief Destroy existing screen layer.
    void destroy_layer(SUIDRegistry idReg, SUID id);

    /// @brief Rename an existing screen layer.
    void rename_layer(SUID id, const char* name);

    /// @brief Move layer to index, shifting other layers.
    void rotate_layer(SUID id, int index);

    /// @brief Retrieve all screen layers ordered.
    Vector<ProjectScreenLayer> get_layers();
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

    /// @brief Get interface for rendering settings.
    ProjectRenderingSettings get_rendering_settings();

    /// @brief Get interface for screen layer settings.
    ProjectScreenLayerSettings get_screen_layer_settings();
};

} // namespace LD