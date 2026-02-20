#include <Ludens/Camera/Camera2D.h>
#include <Ludens/Memory/Memory.h>

namespace LD {

struct Camera2DObj
{
    Mat4 view;
    Mat4 proj;
    Vec2 pos{};
    Vec2 halfExtent{};
    float zoom = 1.0f;
    float rot = 0.0f;
    bool isViewDirty = true;
    bool isProjDirty = true;
};

Camera2D Camera2D::create(const Vec2& extent)
{
    Camera2DObj* obj = (Camera2DObj*)heap_new<Camera2DObj>(MEMORY_USAGE_MISC);
    obj->halfExtent = extent * 0.5f;
    obj->zoom = 1.0f;
    obj->rot = 0.0f;
    obj->pos = obj->halfExtent;
    obj->isViewDirty = true;
    obj->isProjDirty = true;

    return Camera2D(obj);
}

void Camera2D::destroy(Camera2D camera)
{
    Camera2DObj* obj = camera.unwrap();

    heap_delete<Camera2DObj>(obj);
}

void Camera2D::set_extent(const Vec2& extent)
{
    mObj->halfExtent = extent * 0.5f;
}

Vec2 Camera2D::get_extent()
{
    return mObj->halfExtent * 2.0f;
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

void Camera2D::set_zoom(float zoom)
{
    LD_ASSERT(zoom != 0.0f);

    mObj->zoom = zoom;
    mObj->isProjDirty = true;
}

float Camera2D::get_zoom()
{
    return mObj->zoom;
}

Vec2 Camera2D::get_world_position(const Vec2& screenPos)
{
    return mObj->pos + (screenPos - mObj->halfExtent) / mObj->zoom;
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
    if (mObj->isProjDirty)
    {
        mObj->isProjDirty = false;
        float w = mObj->halfExtent.x / mObj->zoom;
        float h = mObj->halfExtent.y / mObj->zoom;
        mObj->proj = Mat4::orthographic(-w, w, h, -h, -1.0f, 1.0f);
    }

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