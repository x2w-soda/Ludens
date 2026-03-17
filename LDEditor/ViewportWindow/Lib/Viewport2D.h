#pragma once

#include <Ludens/Camera/Camera2D.h>
#include <Ludens/Camera/Camera2DController.h>
#include <Ludens/DSA/Optional.h>
#include <LudensEditor/EditorContext/EditorContext.h>

#include "ViewportCommon.h"

namespace LD {

/// @brief Viewport 2D mode, manage Scene 2D elements.
class Viewport2D
{
public:
    Viewport2D() = default;
    Viewport2D(const Viewport2D&) = delete;
    ~Viewport2D();

    Viewport2D& operator=(const Viewport2D&) = delete;

    void create(EditorContext ctx);
    void destroy();

    void imgui(const ViewportState& state);

    /// @brief 2D viewport camera.
    inline Camera2D get_camera_2d() { return mCamera; }

private:
    void drag_controls(const ViewportState& state);

private:
    EditorContext mCtx;
    Camera2D mCamera;
    Camera2DController mCameraController;
    Optional<Vec2> mMouseScenePos; // mouse screen position in scene extent
    Optional<Vec2> mMouseWorldPos; // mouse world position
    Vec2 mDragPosPrevFrame;
    Vec2 mDragPosThisFrame;
    Vec2 mDragMouseDelta;
    bool mIsPanning = false;
    bool mIsDragging = false;
};

} // namespace LD