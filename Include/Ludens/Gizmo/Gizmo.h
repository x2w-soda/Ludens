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

/// @brief 3D Gizmo controller to edit the Translation, Rotation, and Scale
///        properties of an object in world space.
struct Gizmo : Handle<struct GizmoObj>
{
    /// @brief Create gizmo controller.
    static Gizmo create();

    /// @brief Destroy gizmo controller.
    static void destroy(Gizmo gizmo);

    /// @brief Check if the gizmo is currently active.
    /// @param axis If gizmo control uses an axis, return the active axis.
    /// @param plane If gizmo control uses a plane, return the active plane.
    /// @return The current control or GIZMO_CONTROL_NONE.
    GizmoControl is_active(GizmoAxis& axis, GizmoPlane& plane) const;

    /// @brief Stop gizmo manipulation.
    void end();

    /// @brief Update camera ray into screen.
    /// @param camera Camera handle.
    /// @param screenPos Screen space position to cast ray from, usually the mouse cursor position.
    /// @param screenSize Screen extent.
    void update(Camera camera, const Vec2& screenPos, const Vec2& screenSize);

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