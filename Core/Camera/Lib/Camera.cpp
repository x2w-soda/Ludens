#include "CameraCommon.h"
#include <Ludens/Camera/Camera.h>
#include <Ludens/System/Memory.h>

namespace LD {

struct CameraObj
{
    Mat4 view;
    Mat4 proj;
    Vec3 pos;
    Vec3 target;
    Vec3 worldUp;
    union
    {
        CameraPerspectiveInfo perspective;
        CameraOrthographicInfo ortho;
    };
    bool isViewDirty;
    bool isPerspective;
};

Camera Camera::create(const CameraPerspectiveInfo& perspectiveInfo, const Vec3& target)
{
    CameraObj* obj = (CameraObj*)heap_new<CameraObj>(MEMORY_USAGE_MISC);
    Camera handle(obj);

    obj->pos = Vec3(0.0f);
    obj->target = target;
    obj->worldUp = CAMERA_WORLD_UP;
    obj->view = Mat4::look_at(obj->pos, obj->target, obj->worldUp);
    obj->isViewDirty = false;

    handle.set_perspective(perspectiveInfo);

    return handle;
}

Camera Camera::create(const CameraOrthographicInfo& orthographicInfo, const Vec3& target)
{
    CameraObj* obj = (CameraObj*)heap_new<CameraObj>(MEMORY_USAGE_MISC);
    Camera handle(obj);

    obj->pos = Vec3(0.0f);
    obj->target = target;
    obj->worldUp = CAMERA_WORLD_UP;
    obj->view = Mat4::look_at(obj->pos, obj->target, obj->worldUp);
    obj->isViewDirty = false;

    handle.set_orthographic(orthographicInfo);

    return handle;
}

void Camera::destroy(Camera camera)
{
    CameraObj* obj = camera;

    heap_delete<CameraObj>(obj);
}

void Camera::set_pos(const Vec3& pos)
{
    mObj->isViewDirty = true;
    mObj->pos = pos;
}

void Camera::set_target(const Vec3& target)
{
    mObj->isViewDirty = true;
    mObj->target = target;
}

void Camera::set_up_vector(const Vec3& up)
{
    mObj->isViewDirty = true;
    mObj->worldUp = up;
}

void Camera::set_perspective(const CameraPerspectiveInfo& perspectiveInfo)
{
    mObj->perspective = perspectiveInfo;
    mObj->proj = Mat4::perspective(mObj->perspective.fov, mObj->perspective.aspectRatio, mObj->perspective.nearClip, mObj->perspective.farClip);
    mObj->isPerspective = true;
}

void Camera::set_orthographic(const CameraOrthographicInfo& orthographicInfo)
{
    mObj->ortho = orthographicInfo;
    mObj->proj = Mat4::orthographic(mObj->ortho.left, mObj->ortho.right, mObj->ortho.bottom, mObj->ortho.top, mObj->ortho.nearClip, mObj->ortho.farClip);
    mObj->isPerspective = false;
}

bool Camera::is_perspective() const
{
    return mObj->isPerspective;
}

const Vec3& Camera::get_pos() const
{
    return mObj->pos;
}

const Vec3& Camera::get_target() const
{
    return mObj->target;
}

const Mat4& Camera::get_proj() const
{
    return mObj->proj;
}

const Mat4& Camera::get_view() const
{
    if (mObj->isViewDirty)
    {
        mObj->isViewDirty = false;
        mObj->view = Mat4::look_at(mObj->pos, mObj->target, mObj->worldUp);
    }

    return mObj->view;
}

const Mat4& Camera::get_view_proj() const
{
    return mObj->proj * get_view();
}

} // namespace LD