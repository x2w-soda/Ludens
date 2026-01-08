#pragma once

#include <Ludens/DSA/Vector.h>
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

    std::string get_default_scene_path();
    void set_default_scene_path(const std::string& scenePath);
};

/// @brief Uniquely identifies a screen layer, invariant to rename operations.
using ProjectScreenLayerID = uint32_t;

struct ProjectScreenLayer
{
    ProjectScreenLayerID id;
    std::string name;
};

/// @brief Settings for 2D screen layers.
struct ProjectScreenLayerSettings : Handle<struct ProjectSettingsObj>
{
    /// @brief Create a new screen layer.
    ProjectScreenLayerID create_layer(const char* name);

    /// @brief Destroy existing screen layer.
    void destroy_layer(ProjectScreenLayerID id);

    /// @brief Rename an existing screen layer.
    void rename_layer(ProjectScreenLayerID id, const char* name);

    /// @brief Move layer to index, shifting other layers.
    void rotate_layer(ProjectScreenLayerID id, int index);

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

    /// @brief Get interface for screen layer settings.
    ProjectScreenLayerSettings get_screen_layer_settings();
};

} // namespace LD