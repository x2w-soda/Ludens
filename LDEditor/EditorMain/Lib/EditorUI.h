#pragma once

#include "EditorBottomBar.h"
#include "EditorTopBar.h"
#include <Ludens/Media/Font.h>
#include <Ludens/RenderComponent/SceneOverlayComponent.h>
#include <Ludens/UI/UIContext.h>
#include <Ludens/UI/UIWindow.h>
#include <Ludens/UI/UIWindowManager.h>
#include <LudensEditor/EInspectorWindow/EInspectorWindow.h>
#include <LudensEditor/EOutlinerWindow/EOutlinerWindow.h>
#include <LudensEditor/EViewportWindow/EViewportWindow.h>
#include <LudensEditor/EditorContext/EditorContext.h>
#include <LudensEditor/EditorWidget/UISelectWindow.h>
#include <LudensEditor/EditorWidget/UIVersionWindow.h>
#include <cstdint>

namespace LD {

struct Event;

struct EditorUIInfo
{
    EditorContext ctx;
    uint32_t screenWidth;
    uint32_t screenHeight;
    uint32_t barHeight;
    FontAtlas fontAtlas;
    RImage fontAtlasImage;
};

/// @brief Editor user interface implementation. Organizes windows via
///        the UIWindowManager and provides callbacks for the render server.
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

    void get_viewport_gizmo_state(SceneOverlayGizmo& gizmoType, Vec3& gizmoCenter, float& gizmoScale, RServerSceneGizmoColor& gizmoColor)
    {
        mViewportWindow.get_gizmo_state(gizmoType, gizmoCenter, gizmoScale, gizmoColor);
    }

    void show_version_window();

    struct SelectWindowUsage
    {
        void (*onSelect)(const FS::Path& path, void* user);
        void (*onCancel)(void* user);
        void* user;
    };

    void show_select_window(const SelectWindowUsage& usage);

    static void on_event(const Event* event, void* user);
    static void on_render(ScreenRenderComponent renderer, void* user);
    static void on_overlay_render(ScreenRenderComponent renderer, void* user);
    static void on_scene_pick(SceneOverlayGizmoID gizmoID, RUID ruid, void* user);

    /// @brief Editor callback implementations.
    struct ECB
    {
        static void select_asset(AssetType type, AUID currentID, void* user);

        /// @brief Open dialog to add script to component.
        static void add_script_to_component(CUID compID, void* user);
    };

private:
    EditorContext mCtx;
    EditorTopBar mTopBar;
    EditorBottomBar mBottomBar;
    UIWindowManager mWM;
    UISelectWindow mSelectWindow;
    UIVersionWindow mVersionWindow;
    EViewportWindow mViewportWindow;
    EOutlinerWindow mOutlinerWindow;
    EInspectorWindow mInspectorWindow;
    UIWMAreaID mVersionWindowID;
    UIWMAreaID mSelectWindowID;

    struct CallbackState
    {
        CUID compID;
    } mState;
};

} // namespace LD