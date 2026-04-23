#pragma once

#include <Ludens/Header/Math/Vec2.h>

#define LD_SCENE_URI_SCHEME "ld"
#define LD_SCENE_URI_AUTHORITY "scene"
#define LD_SCENE_URI_SCHEME_AUTHORITY "ld://scene/"
#define LD_SCENE_DEFAULT_SCHEMA_FILE_NAME "scene.toml"

namespace LD {

struct SceneUpdateTick
{
    float delta; // delta time in seconds
    Vec2 extent; // screen space extent of the scene
};

} // namespace LD