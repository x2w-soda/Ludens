#pragma once

#include <Ludens/Camera/Camera.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/Header/Math/Transform.h>
#include <Ludens/RenderServer/RenderServer.h>
#include <Ludens/UI/UIWindow.h>
#include <Ludens/UI/UIWindowManager.h>
#include <LudensEditor/EditorContext/EditorContext.h>

namespace LD {

struct EViewportWindowInfo
{
    EditorContext ctx;
    UIWindowManager wm; /// window manager handle
    UIWMAreaID areaID;  /// designated window area
};

/// @brief Editor scene viewport window.
///        Displays the game scene.
struct EViewportWindow : Handle<struct EViewportWindowObj>
{
    /// @brief Create a viewport window
    /// @param windowInfo Viewport window creation info
    /// @return Editor viewport window handle
    static EViewportWindow create(const EViewportWindowInfo& windowInfo);

    /// @brief Destroy viewport window.
    static void destroy(EViewportWindow viewport);

    /// @brief Get the camera for rendering the scene in the viewport window.
    Camera get_editor_camera();

    /// @brief Get viewport window size. This includes toolbar space and
    ///        the scene itself.
    Vec2 get_size();

    /// @brief Get scene extent inside the viewport window.
    Vec2 get_scene_size();

    /// @brief Check for mouse position in the scene viewport
    /// @param pos Mouse position within the viewport window
    /// @return true if mouse cursor is within viewport window
    bool get_mouse_pos(Vec2& pos);

    /// @brief Get the current gizmo control state.
    void get_gizmo_state(SceneOverlayGizmo& gizmoType, Vec3& gizmoCenter, float& gizmoScale, RenderServerSceneGizmoColor& gizmoColor);

    /// @brief Notify the viewport what ID is under the mouse cursor
    /// @param gizmoID If not zero, the gizmo mesh under cursor
    /// @param ruid If not zero, the mesh under cursor
    void hover_id(SceneOverlayGizmoID gizmoID, RUID ruid);
};

} // namespace LD