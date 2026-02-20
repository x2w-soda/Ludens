#pragma once

#include <Ludens/Header/Math/Mat4.h>
#include <Ludens/Header/Math/Rect.h>
#include <Ludens/Header/Math/Vec4.h>

namespace LD {

struct Viewport
{
    Mat4 viewMat; /// view matrix
    Mat4 projMat; /// projection matrix
    Vec4 viewPos; /// view position or direction in homogeneous coordinates
    Rect region;  /// normalized viewport region

    /// @brief Make a 2D viewport from extent.
    static Viewport from_extent(const Vec2& extent)
    {
        Viewport viewport;
        viewport.viewMat = Mat4(1.0f);
        viewport.projMat = Mat4::orthographic_extent(extent);
        viewport.viewPos = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
        viewport.region = Rect(0.0f, 0.0f, 1.0f, 1.0f);
        return viewport;
    }
};

} // namespace LD