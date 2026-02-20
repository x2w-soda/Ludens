#include <Ludens/Camera/Camera2D.h>
#include <Ludens/Memory/Memory.h>

namespace LD {

struct Camera2DObj
{
    Mat4 view;
    Mat4 proj;
    Vec2 pos{};
    float rot = 0.0f;
    bool isViewDirty = true;
};

Camera2D Camera2D::create(const Camera2DInfo& info)
{
    Camera2DObj* obj = (Camera2DObj*)heap_new<Camera2DObj>(MEMORY_USAGE_MISC);

    obj->proj = Mat4::orthographic(info.left, info.right, info.bottom, info.top, info.nearClip, info.farClip);

    return Camera2D(obj);
}

void Camera2D::destroy(Camera2D camera)
{
    Camera2DObj* obj = camera.unwrap();

    heap_delete<Camera2DObj>(obj);
}

void Camera2D::set_position(const Vec2& pos)
{
    mObj->pos = pos;
    mObj->isViewDirty = true;
}

Vec2 Camera2D::get_position()
{
    return mObj->pos;
}

void Camera2D::set_rotation(float rot)
{
    mObj->rot = rot;
    mObj->isViewDirty = true;
}

float Camera2D::get_rotation()
{
    return mObj->rot;
}

Mat4 Camera2D::get_view()
{
    if (mObj->isViewDirty)
    {
        mObj->isViewDirty = false;
        mObj->view = Mat4::rotate(LD_TO_RADIANS(-mObj->rot), Vec3(0.0f, 0.0f, 1.0f)) * Mat4::translate(Vec3(-mObj->pos, 0.0f));
    }

    return mObj->view;
}

Mat4 Camera2D::get_proj()
{
    return mObj->proj;
}

Viewport Camera2D::get_viewport()
{
    Viewport viewport;
    viewport.viewMat = get_view();
    viewport.projMat = get_proj();
    viewport.viewPos = Vec4(mObj->pos, 0.0f, 1.0f);
    viewport.region = Rect(0.0f, 0.0f, 1.0f, 1.0f);
    return viewport;
}

} // namespace LD