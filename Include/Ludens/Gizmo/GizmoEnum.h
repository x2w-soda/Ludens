#pragma once

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

} // namespace LD