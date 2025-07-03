#include <Ludens/Gizmo/Gizmo.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Math/Geometry.h>
#include <Ludens/System/Memory.h>
#include <algorithm>
#include <cmath>

namespace LD {

static Ray get_camera_ray(const Camera& camera, const Vec2& screenPos, const Vec2& screenSize);
static Ray get_axis_ray(Vec3 origin, GizmoAxis axis);
static Vec3 get_axis_unit(GizmoAxis axis);
static Plane get_plane(Vec3 point, GizmoPlane plane);
static GizmoAxis get_plane_complement(GizmoPlane plane);

struct GizmoObj
{
    Vec3 targetPos;
    Vec3 targetScale;
    Vec3 dragOffset;
    Vec3 baseRotation;
    Vec3 lastValidPos;
    Vec3 lastValidScale;
    float lastValidRotation;
    float targetAngleRad;
    GizmoAxis activeAxis;
    GizmoPlane activePlane;
    GizmoControl activeControl;
    Ray cameraRay;
};

Gizmo Gizmo::create()
{
    GizmoObj* obj = (GizmoObj*)heap_malloc(sizeof(GizmoObj), MEMORY_USAGE_MISC);
    obj->activeControl = GIZMO_CONTROL_NONE;

    return {obj};
}

void Gizmo::destroy(Gizmo gizmo)
{
    GizmoObj* obj = gizmo;

    heap_free(obj);
}

GizmoControl Gizmo::is_active(GizmoAxis& axis, GizmoPlane& plane) const
{
    axis = mObj->activeAxis;
    plane = mObj->activePlane;
    return mObj->activeControl;
}

void Gizmo::end()
{
    mObj->activeControl = GIZMO_CONTROL_NONE;
}

void Gizmo::update(const Camera& camera, const Vec2& screenPos, const Vec2& screenSize)
{
    mObj->cameraRay = get_camera_ray(camera, screenPos, screenSize);
}

void Gizmo::begin_axis_translate(GizmoAxis axis, const Vec3& targetPos)
{
    mObj->activeAxis = axis;
    mObj->activeControl = GIZMO_CONTROL_AXIS_TRANSLATION;
    mObj->targetPos = targetPos;
    mObj->lastValidPos = targetPos;

    Ray axisRay = get_axis_ray(mObj->targetPos, mObj->activeAxis);

    float t0, t1;
    bool isParallel = !geometry_nearest(mObj->cameraRay, axisRay, t0, t1);

    if (isParallel)
    {
        mObj->activeControl = GIZMO_CONTROL_NONE;
        return;
    }

    Vec3 nearestPos = axisRay.parametric(t1);
    mObj->dragOffset = nearestPos - mObj->targetPos;
}

Vec3 Gizmo::get_axis_translate()
{
    if (mObj->activeControl != GIZMO_CONTROL_AXIS_TRANSLATION)
        return {};

    Ray axisRay = get_axis_ray(mObj->targetPos, mObj->activeAxis);

    float t0, t1;
    bool isParallel = !geometry_nearest(mObj->cameraRay, axisRay, t0, t1);

    if (isParallel)
        return mObj->lastValidPos;

    Vec3 nearestPos = axisRay.parametric(t1);
    mObj->lastValidPos = nearestPos - mObj->dragOffset;

    return mObj->lastValidPos;
}

void Gizmo::begin_plane_translate(GizmoPlane gizmoPlane, const Vec3& targetPos)
{
    mObj->activePlane = gizmoPlane;
    mObj->activeControl = GIZMO_CONTROL_PLANE_TRANSLATION;
    mObj->targetPos = targetPos;
    mObj->lastValidPos = targetPos;

    Plane targetPlane = get_plane(targetPos, gizmoPlane);

    float t;
    bool isParallel = !geometry_intersects(targetPlane, mObj->cameraRay, t);

    if (isParallel)
    {
        mObj->activeControl = GIZMO_CONTROL_NONE;
        return;
    }

    Vec3 nearestPos = mObj->cameraRay.parametric(t);
    mObj->dragOffset = nearestPos - mObj->targetPos;
}

Vec3 Gizmo::get_plane_translate()
{
    if (mObj->activeControl != GIZMO_CONTROL_PLANE_TRANSLATION)
        return {};

    Plane targetPlane = get_plane(mObj->targetPos, mObj->activePlane);

    float t;
    bool isParallel = !geometry_intersects(targetPlane, mObj->cameraRay, t);

    if (isParallel)
        return mObj->lastValidPos;

    Vec3 nearestPos = mObj->cameraRay.parametric(t);
    mObj->lastValidPos = nearestPos - mObj->dragOffset;

    return mObj->lastValidPos;
}

void Gizmo::begin_plane_rotate(GizmoPlane plane, const Vec3& targetPos, float targetRotation)
{
    mObj->activePlane = plane;
    mObj->activeControl = GIZMO_CONTROL_PLANE_ROTATION;
    mObj->targetPos = targetPos;
    mObj->targetAngleRad = targetRotation;
    mObj->lastValidRotation = targetRotation;

    GizmoAxis rotAxis = get_plane_complement(plane);
    Plane rotPlane;
    rotPlane.dir = get_axis_unit(rotAxis);
    rotPlane.point = targetPos;

    float t;
    bool isParallel = !geometry_intersects(rotPlane, mObj->cameraRay, t);

    if (isParallel)
    {
        mObj->activeControl = GIZMO_CONTROL_NONE;
        return;
    }

    // extract the base rotation vector, a unit vector laying on the rotation plane
    Vec3 nearestPos = mObj->cameraRay.parametric(t);
    mObj->baseRotation = Vec3::normalize(nearestPos - targetPos);
}

float Gizmo::get_plane_rotate()
{
    GizmoAxis rotAxis = get_plane_complement(mObj->activePlane);
    Plane rotPlane;
    rotPlane.dir = get_axis_unit(rotAxis);
    rotPlane.point = mObj->targetPos;

    float t;
    bool isParallel = !geometry_intersects(rotPlane, mObj->cameraRay, t);

    if (isParallel)
        return mObj->lastValidRotation;

    Vec3 nearestPos = mObj->cameraRay.parametric(t);
    Vec3 currentRotation = Vec3::normalize(nearestPos - mObj->targetPos);
    float cosTheta = std::clamp(Vec3::dot(mObj->baseRotation, currentRotation), -1.0f, 1.0f);
    float thetaRad = std::acos(cosTheta); // in the range [0, PI]
    float sign = Vec3::dot(Vec3::cross(mObj->baseRotation, currentRotation), rotPlane.dir);

    mObj->lastValidRotation = mObj->targetAngleRad + copysignf(thetaRad, sign);

    return mObj->lastValidRotation;
}

void Gizmo::begin_axis_scale(GizmoAxis axis, const Vec3& targetPos, const Vec3& targetScale)
{
    mObj->activeAxis = axis;
    mObj->activeControl = GIZMO_CONTROL_AXIS_SCALE;
    mObj->targetPos = targetPos;
    mObj->targetScale = targetScale;
    mObj->lastValidScale = targetScale;

    Ray axisRay = get_axis_ray(mObj->targetPos, mObj->activeAxis);

    float t0, t1;
    bool isParallel = !geometry_nearest(mObj->cameraRay, axisRay, t0, t1);

    if (isParallel)
    {
        mObj->activeControl = GIZMO_CONTROL_NONE;
        return;
    }

    Vec3 nearestPos = axisRay.parametric(t1);
    mObj->dragOffset = nearestPos - mObj->targetPos;
}

Vec3 Gizmo::get_axis_scale()
{
    Ray axisRay = get_axis_ray(mObj->targetPos, mObj->activeAxis);

    float t0, t1;
    bool isParallel = !geometry_nearest(mObj->cameraRay, axisRay, t0, t1);

    if (isParallel)
        return mObj->lastValidScale;

    Vec3 nearestPos = axisRay.parametric(t1);
    float length1 = std::max<float>(mObj->dragOffset.length(), 0.001f);
    float length2 = (nearestPos - mObj->targetPos).length();
    Vec3 scale2 = mObj->targetScale;

    switch (mObj->activeAxis)
    {
    case GIZMO_AXIS_X:
        scale2.x = mObj->targetScale.x * (length2 / length1);
        break;
    case GIZMO_AXIS_Y:
        scale2.y = mObj->targetScale.y * (length2 / length1);
        break;
    case GIZMO_AXIS_Z:
        scale2.z = mObj->targetScale.z * (length2 / length1);
        break;
    }

    mObj->lastValidScale = scale2;
    return scale2;
}

static Ray get_camera_ray(const Camera& camera, const Vec2& screenPos, const Vec2& screenSize)
{
    Vec3 worldNear, worldFar;
    camera.unproject(screenPos, screenSize, worldNear, worldFar);

    Ray cameraRay;
    cameraRay.origin = camera.get_pos();
    cameraRay.dir = Vec3::normalize(worldFar - worldNear);

    return cameraRay;
}

static Ray get_axis_ray(Vec3 origin, GizmoAxis gizmoAxis)
{
    Ray axisRay;
    axisRay.origin = origin;
    axisRay.dir = get_axis_unit(gizmoAxis);

    return axisRay;
}

static Vec3 get_axis_unit(GizmoAxis axis)
{
    switch (axis)
    {
    case GIZMO_AXIS_X:
        return Vec3(1.0f, 0.0f, 0.0f);
    case GIZMO_AXIS_Y:
        return Vec3(0.0f, 1.0f, 0.0f);
    case GIZMO_AXIS_Z:
        return Vec3(0.0f, 0.0f, 1.0f);
    }

    LD_UNREACHABLE;
    return {};
}

static Plane get_plane(Vec3 point, GizmoPlane gizmoPlane)
{
    Plane plane;
    plane.point = point;

    switch (gizmoPlane)
    {
    case GIZMO_PLANE_XY:
        plane.dir = Vec3(0.0f, 0.0f, 1.0f);
        break;
    case GIZMO_PLANE_XZ:
        plane.dir = Vec3(0.0f, 1.0f, 0.0f);
        break;
    case GIZMO_PLANE_YZ:
        plane.dir = Vec3(1.0f, 0.0f, 0.0f);
        break;
    default:
        LD_UNREACHABLE;
    }

    return plane;
}

// get the complement axis of a plane
GizmoAxis get_plane_complement(GizmoPlane gizmoPlane)
{
    switch (gizmoPlane)
    {
    case GIZMO_PLANE_XY:
        return GIZMO_AXIS_Z;
    case GIZMO_PLANE_XZ:
        return GIZMO_AXIS_Y;
    case GIZMO_PLANE_YZ:
        return GIZMO_AXIS_X;
    }

    LD_UNREACHABLE;
    return {};
}

} // namespace LD