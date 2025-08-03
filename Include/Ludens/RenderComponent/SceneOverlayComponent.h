#pragma once

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

    /// @brief get the name of the input scene color attachment
    inline const char* in_color_name() const { return "in_color"; }

    /// @brief get the name of the input scene id-flags color attachment
    inline const char* in_idflags_name() const { return "in_idflags"; }

    /// @brief get the name of the output scene with overlays
    inline const char* out_color_name() const { return "out_color"; }

    /// @brief get the name of the output scene id-flags after rendering gizmos
    inline const char* out_idflags_name() const { return "out_idflags"; }
};

} // namespace LD