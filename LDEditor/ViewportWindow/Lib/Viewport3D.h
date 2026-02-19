#pragma once

#include <Ludens/Camera/Camera.h>
#include <Ludens/Camera/CameraController.h>
#include <Ludens/Gizmo/Gizmo.h>
#include <Ludens/Header/Math/Transform.h>
#include <Ludens/Header/Math/Vec3.h>
#include <LudensEditor/EditorContext/EditorContext.h>

#include "ViewportCommon.h"

namespace LD {

/// @brief Viewport 3D mode, manage Scene 3D elements.
class Viewport3D
{
public:
    Viewport3D() = default;
    Viewport3D(const Viewport3D&) = delete;
    ~Viewport3D();

    Viewport3D& operator=(const Viewport3D&) = delete;

    void create(EditorContext ctx, const Vec2& sceneExtent);
    void destroy();

    void imgui(ViewportState& state);

    /// @brief 3D viewport camera.
    inline Camera get_camera() { return mCamera; }

    /// @brief Get 3D gizmo state, returns GIZMO_CONTROL_NONE if inactive.
    inline GizmoControl get_gizmo_state(Vec3& gizmoCenter, float& gizmoScale, GizmoAxis& axis, GizmoPlane& plane)
    {
        gizmoCenter = mGizmoCenter;
        gizmoScale = mGizmoScale;
        return mGizmo.is_active(axis, plane);
    }

private:
    /// @brief Pick an object in the viewport, could be gizmo mesh or component in scene.
    ///        Updates EditorContext selected component state and notifies observers
    void pick_hover_ruid(ViewportState& state);

    /// @brief Begin 3D gizmo controls in the viewport
    void pick_hover_gizmo_id(ViewportState& state);

    void drag(ViewportState& state, MouseButton btn, const Vec2& dragPos, bool begin);

private:
    EditorContext mCtx;
    TransformEx mSubjectWorldTransform;
    CameraController mCameraController;
    CameraPerspectiveInfo mCameraPerspective;
    Gizmo mGizmo;
    float mGizmoScale;
    Vec3 mGizmoCenter;
    Camera mCamera;
    bool mEnableCameraControls = false;
};

} // namespace LD