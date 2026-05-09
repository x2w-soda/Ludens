#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/DataRegistry/DataRegistry.h>
#include <Ludens/Header/View.h>
#include <Ludens/Scene/Scene.h>

#include <cstdint>
#include <string>

namespace LD {

/// @brief Schema for defining a Scene under the current framework version.
struct SceneSchema
{
    /// @brief Load a scene from TOML schema source string.
    static bool load_scene_from_source(Scene scene, SUIDRegistry idRegistry, const View& toml, String& err);

    /// @brief Load a scene from TOML schema file on disk.
    static bool load_scene_from_file(Scene scene, SUIDRegistry idRegistry, const FS::Path& tomlPath, String& err);

    /// @brief Try saving scene as TOML schema file on disk.
    static bool save_scene(Scene scene, const FS::Path& savePath, String& err);

    static String create_empty();
};

} // namespace LD