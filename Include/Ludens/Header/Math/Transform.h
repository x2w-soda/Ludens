#pragma once

#include <Ludens/Header/Math/Vec2.h>
#include <Ludens/Header/Math/Vec3.h>

namespace LD {

/// @brief Transformation in 3D space
struct Transform
{
    Vec3 rotation; /// rotation in 3 axis
    Vec3 position; /// world position in 3D space
    Vec3 scale;    /// scale in 3 axis
};

/// @brief Transformation in 2D space
struct Transform2D
{
    Vec2 position;  /// world position in 2D space
    Vec2 scale;     /// scale in 2 axis
    float rotation; /// counter-clockwise rotation
};

} // namespace LD