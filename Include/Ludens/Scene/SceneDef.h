#pragma once

#include <Ludens/Header/Math/Vec2.h>

namespace LD {

struct SceneUpdateTick
{
    float delta; // delta time in seconds
    Vec2 extent; // screen space extent of the scene
};

} // namespace LD