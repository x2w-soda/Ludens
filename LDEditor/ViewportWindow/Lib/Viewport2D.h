#pragma once

#include <Ludens/Camera/Camera2D.h>
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

    void create(EditorContext ctx, const Vec2& sceneExtent);
    void destroy();

    void imgui(const ViewportState& state);

    /// @brief 2D viewport camera.
    inline Camera2D get_camera_2d() { return mCamera; }

private:
    EditorContext mCtx;
    Camera2D mCamera;
};

} // namespace LD