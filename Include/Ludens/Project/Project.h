#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/Project/ProjectSettings.h>
#include <Ludens/System/FileSystem.h>
#include <filesystem>
#include <string>

namespace LD {

/// @brief Ludens project handle.
struct Project : Handle<struct ProjectObj>
{
    /// @brief Create empty project.
    static Project create(const FS::Path& rootPath);

    /// @brief Destroy project.
    static void destroy(Project project);

    /// @brief Get project version.
    /// @param major Major semantic version
    /// @param minor Minor semantic version
    /// @param patch Patch semantiv version
    void get_version(int& major, int& minor, int& patch);

    /// @brief Set project name.
    void set_name(const std::string& name);

    /// @brief Get project name.
    std::string get_name() const;

    /// @brief Get path to directory containing Project schema.
    FS::Path get_root_path() const;

    /// @brief Set relative path to project Assets schema.
    /// @param assetsPath Relative path to project root.
    void set_assets_path(const FS::Path& assetsPath);

    /// @brief Get path to Assets schema.
    /// @return Path to Assets schema after concatenating project root path.
    FS::Path get_assets_path() const;

    /// @brief Add relative path to a Scene schema.
    /// @param scenePath Relative path to project root.
    void add_scene_path(const FS::Path& scenePath);

    /// @brief Get paths to scene schemas.
    /// @param scenePaths Outputs paths to scene schemas after concatenating project root path.
    void get_scene_paths(std::vector<FS::Path>& scenePaths) const;

    /// @brief Get interface to project settings.
    ProjectSettings get_settings();
};

} // namespace LD