#include <Ludens/Camera/Camera.h>
#include <Ludens/Memory/Memory.h>

#include "CameraCommon.h"

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

float Camera::screen_to_world_size(const Vec3& worldPos, float screenSizeY, float desiredScreenSizeY) const
{
    float worldSize = 0.0f;

    if (mObj->isPerspective)
    {
        float fovRadians = mObj->perspective.fov;
        Vec3 cameraPos = mObj->pos;
        Vec3 forward = Vec3::normalize(mObj->target - cameraPos);
        const float viewDepth = Vec3::dot(worldPos - cameraPos, forward); // TODO: handle negative
        const float frustumHeight = 2.0f * viewDepth * LD_TAN(fovRadians * 0.5f);
        const float worldSizePerPixel = frustumHeight / screenSizeY;
        worldSize = desiredScreenSizeY * worldSizePerPixel;
    }
    else // orthographic
    {
        const float frustumHeight = std::abs(mObj->ortho.top - mObj->ortho.bottom);
        const float worldSizePerPixel = frustumHeight / screenSizeY;
        worldSize = desiredScreenSizeY * worldSizePerPixel;
    }

    return worldSize;
}

void Camera::unproject(const Vec2& screenPos, const Vec2& screenSize, Vec3& worldNear, Vec3& worldFar)
{
    float ndcX = (screenPos.x / screenSize.x) * 2.0f - 1.0f;
    float ndcY = (screenPos.y / screenSize.y) * 2.0f - 1.0f;

    // near and far plane depends on Mat4.h implementation
    Vec4 nearPos(ndcX, ndcY, -1.0f, 1.0f);
    Vec4 farPos(ndcX, ndcY, +1.0f, 1.0f);

    Mat4 viewProj = get_view_proj();
    Mat4 invViewProj = Mat4::inverse(viewProj);

    Vec4 worldPos = invViewProj * nearPos;
    worldNear = Vec3(worldPos.x, worldPos.y, worldPos.z) / worldPos.w;

    worldPos = invViewProj * farPos;
    worldFar = Vec3(worldPos.x, worldPos.y, worldPos.z) / worldPos.w;
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

void Camera::set_aspect_ratio(float ar)
{
    LD_ASSERT(mObj->isPerspective);

    mObj->perspective.aspectRatio = ar;
    mObj->proj = Mat4::perspective(mObj->perspective.fov, mObj->perspective.aspectRatio, mObj->perspective.nearClip, mObj->perspective.farClip);
}

const CameraPerspectiveInfo& Camera::get_perspective() const
{
    LD_ASSERT(mObj->isPerspective);

    return mObj->perspective;
}

void Camera::set_orthographic(const CameraOrthographicInfo& orthographicInfo)
{
    mObj->ortho = orthographicInfo;
    mObj->proj = Mat4::orthographic(mObj->ortho.left, mObj->ortho.right, mObj->ortho.bottom, mObj->ortho.top, mObj->ortho.nearClip, mObj->ortho.farClip);
    mObj->isPerspective = false;
}

const CameraOrthographicInfo& Camera::get_orthographic() const
{
    LD_ASSERT(!mObj->isPerspective);

    return mObj->ortho;
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

Mat4 Camera::get_view()
{
    if (mObj->isViewDirty)
    {
        mObj->isViewDirty = false;
        mObj->view = Mat4::look_at(mObj->pos, mObj->target, mObj->worldUp);
    }

    return mObj->view;
}

Mat4 Camera::get_view_proj()
{
    return mObj->proj * get_view();
}

} // namespace LD