#pragma once

#include <Ludens/Gizmo/Gizmo.h>
#include <Ludens/Header/Color.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/Header/Math/Vec3.h>
#include <Ludens/RenderGraph/RGraph.h>

namespace LD {

enum SceneOverlayGizmo
{
    SCENE_OVERLAY_GIZMO_NONE = 0,
    SCENE_OVERLAY_GIZMO_TRANSLATION,
    SCENE_OVERLAY_GIZMO_ROTATION,
    SCENE_OVERLAY_GIZMO_SCALE,
};

/// @brief Gizmo ID written to the IDFlags attachment on top of scene
enum SceneOverlayGizmoID
{
    SCENE_OVERLAY_GIZMO_ID_AXIS_X = 1,
    SCENE_OVERLAY_GIZMO_ID_AXIS_Y = 2,
    SCENE_OVERLAY_GIZMO_ID_AXIS_Z = 3,
    SCENE_OVERLAY_GIZMO_ID_PLANE_XY = 4,
    SCENE_OVERLAY_GIZMO_ID_PLANE_XZ = 5,
    SCENE_OVERLAY_GIZMO_ID_PLANE_YZ = 6,
    SCENE_OVERLAY_GIZMO_ID_LAST = SCENE_OVERLAY_GIZMO_ID_PLANE_YZ
};

struct SceneOverlayComponentInfo
{
    RSampleCountBit gizmoMSAA;   /// the MSAA state of gizmo rendering
    RFormat colorFormat;         /// scene color attachment format
    RFormat depthStencilFormat;  /// scene depth-stencil attachment format
    uint32_t width;              /// scene attachment width
    uint32_t height;             /// scene attachment height
    SceneOverlayGizmo gizmoType; /// gizmo to render in scene
    float gizmoScale;            /// gizmo render scaling if not equal 1.0f
    Vec3 gizmoCenter;            /// gizmo center in world space
    Color gizmoColorX;           /// gizmo color for X axes
    Color gizmoColorY;           /// gizmo color for Y axes
    Color gizmoColorZ;           /// gizmo color for Z axes
    Color gizmoColorXY;          /// gizmo color for XY plane
    Color gizmoColorXZ;          /// gizmo color for XZ plane
    Color gizmoColorYZ;          /// gizmo color for YZ plane
};

/// @brief A component to render overlays on top of a scene.
///        Inputs (and Outputs) are the scene colors, scene depth stencils, and the scene id-flags.
///        This component may perform outlining and drawing gizmos on top of an
///        existing scene before it is presented in the scene editor.
struct SceneOverlayComponent : Handle<struct SceneOverlayComponentObj>
{
    /// @brief add the scene overlay component to render graph
    /// @param graph the render graph
    /// @param info scene overlay configuration
    /// @return the component handle
    static SceneOverlayComponent add(RGraph& graph, const SceneOverlayComponentInfo& info);

    /// @brief get the name of the component
    /// @warning returned pointer is transient
    const char* component_name() const;

    RGraphImage in_color_attachment();
    RGraphImage in_id_flags_attachment();
    RGraphImage out_color_attachment();
    RGraphImage out_id_flags_attachment();
};

inline bool get_gizmo_axis(SceneOverlayGizmoID id, GizmoAxis& axis)
{
    switch (id)
    {
    case SCENE_OVERLAY_GIZMO_ID_AXIS_X:
        axis = GIZMO_AXIS_X;
        return true;
    case SCENE_OVERLAY_GIZMO_ID_AXIS_Y:
        axis = GIZMO_AXIS_Y;
        return true;
    case SCENE_OVERLAY_GIZMO_ID_AXIS_Z:
        axis = GIZMO_AXIS_Z;
        return true;
    default:
        break;
    }

    return false;
}

inline bool get_gizmo_plane(SceneOverlayGizmoID id, GizmoPlane& plane)
{
    switch (id)
    {
    case SCENE_OVERLAY_GIZMO_ID_PLANE_XY:
        plane = GIZMO_PLANE_XY;
        return true;
    case SCENE_OVERLAY_GIZMO_ID_PLANE_XZ:
        plane = GIZMO_PLANE_XZ;
        return true;
    case SCENE_OVERLAY_GIZMO_ID_PLANE_YZ:
        plane = GIZMO_PLANE_YZ;
        return true;
    default:
        break;
    }

    return false;
}

inline float get_plane_rotation(GizmoPlane plane, const Vec3& axisRotations)
{
    switch (plane)
    {
    case GIZMO_PLANE_XY:
        return (float)LD_TO_RADIANS(axisRotations.z);
    case GIZMO_PLANE_XZ:
        return (float)LD_TO_RADIANS(axisRotations.y);
    case GIZMO_PLANE_YZ:
        return (float)LD_TO_RADIANS(axisRotations.x);
    }

    LD_UNREACHABLE;
    return 0.0f;
}

} // namespace LD