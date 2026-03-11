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
    static Project create();

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
    std::string get_name();

    /// @brief Get absolute path to directory containing Project schema.
    FS::Path get_root_path();

    /// @brief Set absolute path to project schema.
    void set_schema_path(const FS::Path& projectSchemaPath);

    /// @brief Get absolute path to project schema.
    FS::Path get_schema_path();

    /// @brief Set relative path to asset schema.
    void set_asset_schema_path(const FS::Path& assetSchemaPath);

    /// @brief Get relative path to asset schema.
    /// @return Path to Assets schema after concatenating project root path.
    FS::Path get_asset_schema_path();

    /// @brief Get absolute path to asset schema.
    inline FS::Path get_asset_schema_absolute_path()
    {
        return get_root_path() / get_asset_schema_path();
    }

    /// @brief Add relative path to a Scene schema.
    /// @param scenePath Relative path to project root.
    void add_scene_path(const FS::Path& scenePath);

    /// @brief Get relative paths to scene schemas.
    /// @param scenePaths Outputs paths to scene schemas after concatenating project root path.
    void get_scene_paths(Vector<FS::Path>& scenePaths);

    void get_scene_absolute_paths(Vector<FS::Path>& scenePaths)
    {
        const FS::Path rootPath = get_root_path();
        get_scene_paths(scenePaths);
        for (size_t i = 0; i < scenePaths.size(); i++)
            scenePaths[i] = rootPath / scenePaths[i];
    }

    /// @brief Get interface to project settings.
    ProjectSettings get_settings();
};

} // namespace LD