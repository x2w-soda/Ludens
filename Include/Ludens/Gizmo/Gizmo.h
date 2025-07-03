#pragma once

#include <Ludens/Camera/Camera.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/Header/Math/Vec2.h>
#include <Ludens/Header/Math/Vec3.h>

namespace LD {

enum GizmoControl
{
    GIZMO_CONTROL_NONE = 0,          /// gizmo is not active
    GIZMO_CONTROL_AXIS_TRANSLATION,  /// translating along an axis using axis gizmo
    GIZMO_CONTROL_PLANE_TRANSLATION, /// translating along a plane using plane gizmo
    GIZMO_CONTROL_PLANE_ROTATION,    /// rotating along an axis using its complement plane gizmo
    GIZMO_CONTROL_AXIS_SCALE,        /// scaling along an axis using axis gizmo
};

enum GizmoAxis
{
    GIZMO_AXIS_X = 0,
    GIZMO_AXIS_Y,
    GIZMO_AXIS_Z,
};

enum GizmoPlane
{
    GIZMO_PLANE_XY = 0,
    GIZMO_PLANE_XZ,
    GIZMO_PLANE_YZ,
};

struct Gizmo : Handle<struct GizmoObj>
{
    static Gizmo create();
    static void destroy(Gizmo gizmo);

    /// @brief check if the gizmo is currently active
    /// @param axis if gizmo control uses an axis, return the active axis
    /// @param plane if gizmo control uses a plane, return the active plane
    /// @return the current control or GIZMO_CONTROL_NONE
    GizmoControl is_active(GizmoAxis& axis, GizmoPlane& plane) const;

    /// @brief stop gizmo manipulation
    void end();

    void update(const Camera& camera, const Vec2& screenPos, const Vec2& screenSize);

    void begin_axis_translate(GizmoAxis axis, const Vec3& targetPos);
    Vec3 get_axis_translate();

    void begin_plane_translate(GizmoPlane plane, const Vec3& targetPos);
    Vec3 get_plane_translate();

    void begin_plane_rotate(GizmoPlane plane, const Vec3& targetPos, float targetRotation);
    float get_plane_rotate();

    void begin_axis_scale(GizmoAxis axis, const Vec3& targetPos, const Vec3& targetScale);
    Vec3 get_axis_scale();
};

} // namespace LD