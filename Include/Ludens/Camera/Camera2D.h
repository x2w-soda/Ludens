#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/Header/Math/Mat4.h>
#include <Ludens/Header/Math/Vec2.h>

namespace LD {

struct Camera2DInfo
{
    float left;
    float right;
    float top;
    float bottom;
    float nearClip;
    float farClip;

    static Camera2DInfo extent(float width, float height)
    {
        Camera2DInfo cameraI{};
        cameraI.top = 0.0f;
        cameraI.bottom = height;
        cameraI.left = 0.0f;
        cameraI.right = width;
        cameraI.nearClip = -1.0f;
        cameraI.farClip = 1.0f;
        return cameraI;
    }
};

struct Camera2D : Handle<struct Camera2DObj>
{
    static Camera2D create(const Camera2DInfo& info);
    static void destroy(Camera2D camera);

    void set_position(const Vec2& pos);
    Vec2 get_position();

    void set_rotation(float rotation);
    float get_rotation();

    Mat4 get_view();
    Mat4 get_view_proj();
};

} // namespace LD