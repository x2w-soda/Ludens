#pragma once

#include <Ludens/DSA/Vector.h>
#include <Ludens/Media/Font.h>
#include <Ludens/RenderComponent/SceneOverlayComponent.h>
#include <Ludens/System/FileSystem.h>
#include <Ludens/UI/UIContext.h>
#include <Ludens/UI/UIWindow.h>

#include <LudensEditor/EditorContext/EditorContext.h>
#include <LudensEditor/ConsoleWindow/ConsoleWindow.h>
#include <LudensEditor/InspectorWindow/InspectorWindow.h>
#include <LudensEditor/OutlinerWindow/OutlinerWindow.h>
#include <LudensEditor/SelectionWindow/SelectionWindow.h>
#include <LudensEditor/VersionWindow/VersionWindow.h>
#include <LudensEditor/ViewportWindow/ViewportWindow.h>

#include <LudensEditor/EditorUI/EditorTopBar.h>
#include <LudensEditor/EditorUI/EditorWorkspace.h>

#include <cstdint>

namespace LD {

struct Event;
struct EditorWindowObj;

struct EditorUIInfo
{
    EditorContext ctx;
    uint32_t screenWidth;
    uint32_t screenHeight;
    uint32_t barHeight;
    FontAtlas fontAtlas;
    RImage fontAtlasImage;
};

/// @brief Editor user interface.
///        Owner of the UIContext for all editor UI.
///        Provides mouse-picking callbacks for the render server.
///        Provides modal "selection" windows.
class EditorUI
{
public:
    /// @brief In-place startup of the editor UI
    void startup(const EditorUIInfo& info);

    /// @brief In-place cleanup of the editor UI
    void cleanup();

    /// @brief Updates the editor UI with timestep.
    void update(float delta);

    /// @brief Resize the editor UI to new screen size.
    void resize(const Vec2& screenSize);

    void viewport_hover_id(SceneOverlayGizmoID gizmoID, RUID ruid)
    {
        mViewportWindow.hover_id(gizmoID, ruid);
    }

    EditorContext get_editor_context()
    {
        return mCtx;
    }

    /// @brief Get the main camera used to render scene from.
    Camera get_main_camera()
    {
        Camera sceneCamera;
        if (mCtx.is_playing() && (sceneCamera = mCtx.get_scene_camera()))
            return sceneCamera;

        return get_viewport_camera();
    }

    Camera get_viewport_camera()
    {
        return mViewportWindow.get_editor_camera();
    }

    Vec2 get_viewport_size()
    {
        return mViewportWindow.get_size();
    }

    Vec2 get_viewport_scene_size()
    {
        return mViewportWindow.get_scene_size();
    }

    bool get_viewport_mouse_pos(Vec2& pickPos)
    {
        return mViewportWindow.get_mouse_pos(pickPos);
    }

    RUID get_viewport_outline_ruid()
    {
        return mCtx.get_selected_component_ruid();
    }

    void get_viewport_gizmo_state(SceneOverlayGizmo& gizmoType, Vec3& gizmoCenter, float& gizmoScale, RenderServerSceneGizmoColor& gizmoColor)
    {
        mViewportWindow.get_gizmo_state(gizmoType, gizmoCenter, gizmoScale, gizmoColor);
    }

    /// @brief Begin modal to open new scene.
    void modal_open_scene();

    /// @brief Begin modal to select an asset.
    void modal_select_asset(AssetType type);

    /// @brief Begin modal to select script asset for a component.
    void modal_select_script();

    /// @brief Show version window in float layer.
    void show_version_window();

    static void on_event(const Event* event, void* user);
    static void on_render(ScreenRenderComponent renderer, void* user);
    static void on_render_overlay(ScreenRenderComponent renderer, void* user);
    static void on_scene_pick(SceneOverlayGizmoID gizmoID, RUID ruid, void* user);

    //static void select_asset(AssetType type, AUID currentID, void* user);

    /// @brief Open dialog to add script to component.
    //static void add_script_to_component(CUID compID, void* user);

private:
    enum Modal
    {
        MODAL_NONE = 0,
        MODAL_OPEN_SCENE,
        MODAL_SELECT_ASSET,
        MODAL_SELECT_SCRIPT,
    };

    void update_ground_workspace();
    void update_modal_workspace();

private:
    EditorContext mCtx;
    EditorTopBar mTopBar;
    Modal mModal = MODAL_NONE;
    UIContext mUI{};
    UILayer mUIGroundLayer{};
    UILayer mUIFloatLayer{};
    UILayer mUIModalLayer{};

    CUID mSubjectCompID; // component requesting new asset or script

    EditorWorkspace mSceneWorkspace{};
    ViewportWindow mViewportWindow{};
    OutlinerWindow mOutlinerWindow{};
    InspectorWindow mInspectorWindow{};
    ConsoleWindow mConsoleWindow{};

    EditorWorkspace mFloatWorkspace{};
    VersionWindow mVersionWindow{};

    EditorWorkspace mModalWorkspace{};
    SelectionWindow mSelectionWindow{};
};

} // namespace LD