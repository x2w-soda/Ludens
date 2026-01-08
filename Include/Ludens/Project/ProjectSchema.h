#pragma once

#include <Ludens/Header/View.h>
#include <Ludens/Project/Project.h>
#include <Ludens/System/FileSystem.h>
#include <string>

namespace LD {

/// @brief Schema for defining a Project under the current engine version.
///        While the Project and ProjectSettings lives in RAM, the Schema is
///        meant to be serialized to disk.
struct ProjectSchema
{
    /// @brief Load a project from TOML schema source string.
    static bool load_project_from_source(Project project, const View& toml, std::string& err);

    /// @brief Load a project from TOML schema file on disk.
    static bool load_project_from_file(Project project, const FS::Path& tomlPath, std::string& err);

    /// @brief Try saving project as TOML schema file on disk.
    static bool save_project(Project project, const FS::Path& savePath, std::string& err);
};

} // namespace LD