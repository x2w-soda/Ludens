#pragma once

#include <Ludens/Media/Format/TOML.h>
#include <Ludens/Scene/Scene.h>
#include <string>

namespace LD {

/// @brief Schema for defining a Scene under the current framework version.
struct SceneSchema
{
    /// @brief Load a scene with TOML schema. Assets are not loaded yet but
    ///        the component hierarchy and asset IDs should be in place.
    static void load_scene(Scene scene, TOMLDocument doc);
};

} // namespace LD