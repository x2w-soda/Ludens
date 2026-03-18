#include <Ludens/Gizmo/Gizmo2D.h>

namespace LD {
namespace Gizmo2D {

Transform2D translate(const Transform2D& childLocal, const Transform2D& parentWorld, Vec2 mouseWorldPos, Vec2 startMouseOffset)
{
    Transform2D newChildLocal = childLocal;
    Vec2 targetWorldPos = mouseWorldPos + startMouseOffset;

    Vec2 delta = targetWorldPos - parentWorld.position;
    Vec4 newLocalPos = Mat4::rotate(LD_TO_RADIANS(-parentWorld.rotation), Vec3(0.0f, 0.0f, 1.0f)) * Vec4(delta, 0.0f, 1.0f);

    if (!is_zero_epsilon(parentWorld.scale.x))
        newLocalPos.x /= parentWorld.scale.x;
    if (!is_zero_epsilon(parentWorld.scale.y))
        newLocalPos.y /= parentWorld.scale.y;

    newChildLocal.position = Vec2(newLocalPos.x, newLocalPos.y);

    return newChildLocal;
}

Transform2D rotate(const Transform2D& childLocal, const Transform2D& parentWorld, Vec2 mouseWorldPos, float startMouseOffsetDeg, float startChildWorldDeg)
{
    Transform2D newChildLocal = childLocal;

    Vec2 childWorldPos = Transform2D::child_world(childLocal, parentWorld).position;
    float currentMouseAngleDeg = LD_TO_DEGREES(LD_ATAN2(mouseWorldPos.y - childWorldPos.y, mouseWorldPos.x - childWorldPos.x));
    float deltaDeg = currentMouseAngleDeg - startMouseOffsetDeg;
    float newChildWorldDeg = startChildWorldDeg + deltaDeg;

    newChildLocal.rotation = newChildWorldDeg - parentWorld.rotation;

    return newChildLocal;
}

Transform2D scale(const Transform2D& childLocal, const Transform2D& parentWorld, Vec2 mouseWorldPos, float startDist, Vec2 startChildWorldScale)
{
    Transform2D newChildLocal = childLocal;

    Vec2 childWorldPos = Transform2D::child_world(childLocal, parentWorld).position;
    float currentDist = (mouseWorldPos - childWorldPos).length();

    float ratio = 1.0f;
    if (!is_zero_epsilon(startDist))
        ratio = currentDist / startDist;

    Vec2 newChildWorldScale = startChildWorldScale * ratio;

    newChildLocal.scale.x = 1.0f;
    if (!is_zero_epsilon(parentWorld.scale.x))
        newChildLocal.scale.x = newChildWorldScale.x / parentWorld.scale.x;

    newChildLocal.scale.y = 1.0f;
    if (!is_zero_epsilon(parentWorld.scale.y))
        newChildLocal.scale.y = newChildWorldScale.y / parentWorld.scale.y;

    return newChildLocal;
}

} // namespace Gizmo2D
} // namespace LD