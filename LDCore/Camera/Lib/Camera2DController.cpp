#include <Ludens/Camera/Camera2DController.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Memory/Memory.h>

#include <algorithm>

namespace LD {

struct Camera2DControllerObj
{
    Camera2D subject;
    float zoomExpTarget = 0.0f;
    float zoomExpNow = 0.0f;
    float zoomSensitivity = 0.14f;
};

Camera2DController Camera2DController::create(Camera2D subject)
{
    auto* obj = heap_new<Camera2DControllerObj>(MEMORY_USAGE_MISC);

    obj->subject = subject;
    LD_ASSERT(obj->subject);

    return Camera2DController(obj);
}

void Camera2DController::destroy(Camera2DController controller)
{
    auto* obj = controller.unwrap();

    heap_delete<Camera2DControllerObj>(obj);
}

void Camera2DController::update(float delta, const Vec2* inMousePos)
{
    constexpr float smoothness = 10.0f;
    mObj->zoomExpNow = std::lerp(mObj->zoomExpNow, mObj->zoomExpTarget, delta * smoothness);

    const Vec2 centerPos = mObj->subject.get_extent() * 0.5f;
    const Vec2* mousePos = inMousePos ? inMousePos : &centerPos;
    Vec2 oldMouseWorldPos = mObj->subject.get_world_position(*mousePos);

    mObj->subject.set_zoom(std::pow(2.0f, mObj->zoomExpNow));

    Vec2 newMouseWorldPos = mObj->subject.get_world_position(*mousePos);

    // zoom to cursor
    mObj->subject.set_position(mObj->subject.get_position() + (oldMouseWorldPos - newMouseWorldPos));
}

void Camera2DController::accumulate_zoom_exp(float zoomExpDelta)
{
    mObj->zoomExpTarget += zoomExpDelta * mObj->zoomSensitivity;
    mObj->zoomExpTarget = std::clamp(mObj->zoomExpTarget, -8.0f, 8.0f);
}

} // namespace LD