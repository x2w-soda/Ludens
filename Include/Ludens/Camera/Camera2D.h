#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/Header/Math/Mat4.h>
#include <Ludens/Header/Math/Vec2.h>
#include <Ludens/Header/Math/Viewport.h>

namespace LD {

struct Camera2D : Handle<struct Camera2DObj>
{
    static Camera2D create(const Vec2& extent);
    static void destroy(Camera2D camera);

    void set_extent(const Vec2& extent);
    Vec2 get_extent();

    void set_position(const Vec2& pos);
    Vec2 get_position();

    void set_rotation(float rotation);
    float get_rotation();

    void set_zoom(float zoom);
    float get_zoom();

    /// @brief Get 2D world position from screen position in camera extent,
    ///        taking zoom into account.
    Vec2 get_world_position(const Vec2& screenPos);

    Mat4 get_view();
    Mat4 get_proj();
    Viewport get_viewport();
};

} // namespace LD