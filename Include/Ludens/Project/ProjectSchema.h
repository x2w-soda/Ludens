#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/Project/Project.h>
#include <string>

namespace LD {

/// @brief Schema for defining a Project under the current framework version.
struct ProjectSchema : Handle<struct ProjectSchemaObj>
{
    /// @brief Create schema from TOML source string.
    static ProjectSchema create_from_source(const char* source, size_t len);

    /// @brief Create schema from TOML file on disk.
    static ProjectSchema create_from_file(const FS::Path& tomlPath);

    /// @brief Destroy the project schema.
    static void destroy(ProjectSchema schema);

    /// @brief Get default schema TOML text.
    static std::string get_default_text(const std::string& projectName, const FS::Path& assetSchemaPath);

    /// @brief Load a project from TOML schema
    void load_project(Project project);
};

} // namespace LD