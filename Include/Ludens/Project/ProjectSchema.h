#pragma once

#include <Ludens/Project/Project.h>
#include <string>

namespace LD {

/// @brief Schema for defining a Project under the current engine version.
///        While the Project and ProjectSettings lives in RAM, the Schema is
///        meant to be serialized to disk.
struct ProjectSchema
{
    /// @brief Load a project from TOML schema source string.
    static void load_project_from_source(Project project, const char* source, size_t len);

    /// @brief Load a project from TOML schema file on disk.
    static void load_project_from_file(Project project, const FS::Path& tomlPath);

    /// @brief Get default schema TOML text.
    static std::string get_default_text(const std::string& projectName, const FS::Path& assetSchemaPath);
};

} // namespace LD