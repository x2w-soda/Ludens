#pragma once

#include <Ludens/DSA/String.h>
#include <Ludens/Header/View.h>
#include <Ludens/Project/Project.h>
#include <Ludens/System/FileSystem.h>

namespace LD {

/// @brief Schema for defining a Project under the current engine version.
///        While the Project and ProjectSettings lives in RAM, the Schema is
///        meant to be serialized to disk.
struct ProjectSchema
{
    /// @brief Load a project from TOML schema source string.
    static bool load_project_from_source(Project project, SUIDRegistry idReg, const FS::Path& rootDir, const View& toml, String& err);

    /// @brief Load a project from TOML schema file on disk.
    static bool load_project_from_file(Project project, SUIDRegistry idReg, const FS::Path& tomlPath, String& err);

    /// @brief Try saving project as TOML string.
    static bool save_project_to_string(Project project, String& saveTOML, String& err);

    /// @brief Try saving project as TOML schema file on disk.
    static bool save_project(Project project, const FS::Path& savePath, String& err);

    static String create_empty(View projectName, View assetSchemaRelPath, View sceneURIPath, View sceneName);
};

} // namespace LD