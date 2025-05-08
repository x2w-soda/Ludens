#include "CameraCommon.h"
#include <Ludens/Camera/CameraController.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Math/Math.h>
#include <Ludens/Header/Math/Vec2.h>
#include <Ludens/System/Memory.h>
#include <algorithm>

namespace LD {

struct CameraControllerObj
{
    Camera subject;
    Vec3 viewDir;
    float moveSpeed;
    float viewSpeed; // degrees
    float viewPitch; // degrees
    float viewYaw;
    float frameViewPitch;
    float frameViewYaw;
    char frameMoveForward;
    char frameMoveLeft;
    char frameMoveWorldUp;
};

CameraController CameraController::create(Camera subject, float moveSpeed, float rotSpeed)
{
    LD_ASSERT(subject);

    CameraControllerObj* obj = heap_new<CameraControllerObj>(MEMORY_USAGE_MISC);

    obj->subject = subject;
    obj->moveSpeed = moveSpeed;
    obj->viewSpeed = rotSpeed;
    obj->viewPitch = 0.0f;
    obj->viewYaw = -90.0f;
    obj->viewDir = {0.0f, 0.0f, -1.0f};

    return {obj};
}

void CameraController::destroy(CameraController controller)
{
    CameraControllerObj* obj = controller;

    heap_delete(obj);
}

void CameraController::move_forward()
{
    mObj->frameMoveForward++;
}

void CameraController::move_backward()
{
    mObj->frameMoveForward--;
}

void CameraController::move_left()
{
    mObj->frameMoveLeft++;
}

void CameraController::move_right()
{
    mObj->frameMoveLeft--;
}

void CameraController::move_world_up()
{
    mObj->frameMoveWorldUp++;
}

void CameraController::move_world_down()
{
    mObj->frameMoveWorldUp--;
}

void CameraController::view_pitch(float delta)
{
    mObj->frameViewPitch = delta;
}

void CameraController::view_yaw(float delta)
{
    mObj->frameViewYaw = delta;
}

void CameraController::update(float delta)
{
    Camera cam = mObj->subject;
    Vec3 camPos = cam.get_pos();
    float pitchDelta = 0.0f;
    float yawDelta = 0.0f;

    if (mObj->frameViewPitch != 0.0f)
        pitchDelta = mObj->frameViewPitch * mObj->viewSpeed;

    mObj->frameViewPitch = 0.0f;

    if (mObj->frameViewYaw != 0.0f)
        yawDelta = mObj->frameViewYaw * mObj->viewSpeed;

    mObj->frameViewYaw = 0.0f;

    if (pitchDelta || yawDelta)
    {
        mObj->viewPitch = std::clamp(mObj->viewPitch + pitchDelta, -89.0f, 89.0f);
        mObj->viewYaw += yawDelta;

        mObj->viewDir.y = LD_SIN(LD_TO_RADIANS(mObj->viewPitch));
        mObj->viewDir.x = LD_COS(LD_TO_RADIANS(mObj->viewYaw)) * LD_COS(LD_TO_RADIANS(mObj->viewPitch));
        mObj->viewDir.z = LD_SIN(LD_TO_RADIANS(mObj->viewYaw)) * LD_COS(LD_TO_RADIANS(mObj->viewPitch));
    }

    Vec3 camForward = Vec3::normalize(mObj->viewDir);
    Vec3 camLeft = Vec3::cross(CAMERA_WORLD_UP, camForward);
    Vec3 posDelta(0.0f);

    delta *= mObj->moveSpeed;

    if (mObj->frameMoveForward > 0)
        posDelta += camForward * delta;
    else if (mObj->frameMoveForward < 0)
        posDelta -= camForward * delta;

    mObj->frameMoveForward = 0;

    if (mObj->frameMoveLeft > 0)
        posDelta += camLeft * delta;
    else if (mObj->frameMoveLeft < 0)
        posDelta -= camLeft * delta;

    mObj->frameMoveLeft = 0;

    if (mObj->frameMoveWorldUp > 0)
        posDelta += CAMERA_WORLD_UP * delta;
    else if (mObj->frameMoveWorldUp < 0)
        posDelta -= CAMERA_WORLD_UP * delta;

    mObj->frameMoveWorldUp = 0;

    cam.set_pos(camPos + posDelta);
    cam.set_target(camPos + mObj->viewDir);
}

} // namespace LD