#pragma once

#include <Ludens/Camera/Camera.h>
#include <Ludens/Camera/Camera2D.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/Header/Math/Transform.h>
#include <Ludens/RenderSystem/RenderSystem.h>
#include <Ludens/UI/UIWorkspace.h>
#include <LudensEditor/EditorContext/EditorContext.h>
#include <LudensEditor/EditorContext/EditorWindow.h>

namespace LD {

/// @brief Editor scene viewport window.
///        Displays the game scene.
struct ViewportWindow : Handle<struct ViewportWindowObj>
{
    ViewportWindow() = default;
    ViewportWindow(const EditorWindowObj* obj) { mObj = (ViewportWindowObj*)obj; }

    /// @brief Create a viewport window
    static EditorWindow create(const EditorWindowInfo& windowI);

    /// @brief Destroy viewport window.
    static void destroy(EditorWindow viewport);

    /// @brief Get the camera for rendering the scene in the viewport window.
    Camera get_editor_camera();

    Camera2D get_editor_camera_2d();

    /// @brief Get viewport window size. This includes toolbar space and
    ///        the scene itself.
    Vec2 get_size();

    /// @brief Get scene extent inside the viewport window.
    Vec2 get_scene_size();

    /// @brief Check for mouse position in the scene viewport
    /// @param pos Mouse position within the viewport window
    /// @return true if mouse cursor is within viewport window
    bool get_mouse_pos(Vec2& pos);

    /// @brief Get the current 3D gizmo control state.
    void get_gizmo_3d_state(SceneOverlayGizmo& gizmoType, Vec3& gizmoCenter, float& gizmoScale, RenderSystemSceneGizmoColor& gizmoColor);

    /// @brief Notify the viewport what ID is under the mouse cursor
    /// @param gizmoID If not zero, the gizmo mesh under cursor
    /// @param ruid If not zero, the mesh under cursor
    void hover_id(SceneOverlayGizmoID gizmoID, RUID ruid);
};

} // namespace LD