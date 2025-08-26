#pragma once

#include <Ludens/Media/Format/TOML.h>
#include <Ludens/Project/Project.h>

namespace LD {

/// @brief Schema for defining a Project under the current framework version.
struct ProjectSchema
{
    /// @brief Load a project from TOML schema
    static void load_project(Project project, TOMLDocument doc);
};

} // namespace LD