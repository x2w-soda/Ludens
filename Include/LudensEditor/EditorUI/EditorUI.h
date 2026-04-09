#pragma once

#include <Ludens/Camera/Camera2D.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/Media/Font.h>
#include <Ludens/RenderComponent/SceneOverlayComponent.h>
#include <Ludens/RenderSystem/RenderSystem.h>
#include <Ludens/System/FileSystem.h>
#include <Ludens/UI/UIContext.h>
#include <Ludens/UI/UIWindow.h>

#include <LudensEditor/EditorContext/EditorContext.h>

#include <LudensEditor/EditorUI/EditorDialog.h>
#include <LudensEditor/EditorUI/EditorUIDialog.h>
#include <LudensEditor/EditorUI/EditorUIMain.h>
#include <LudensEditor/EditorUI/EditorUIModal.h>
#include <LudensEditor/EditorUI/EditorUITopBar.h>
#include <LudensEditor/EditorUI/EditorWorkspace.h>

#include <cstdint>

namespace LD {

struct Event;
struct WindowEvent;
struct EditorWindowObj;

struct EditorUIInfo
{
    EditorContext ctx;
    uint32_t screenWidth;
    uint32_t screenHeight;
    uint32_t barHeight;
    RenderSystem renderSystem;
    // RUID envCubemap;
};

/// @brief Editor user interface.
///        Owner of the UIContext for all editor UI.
///        User of the RenderSystem to render both game scene and editor.
class EditorUI
{
public:
    /// @brief In-place startup of the editor UI
    void startup(const EditorUIInfo& info);

    /// @brief In-place cleanup of the editor UI
    void cleanup();

    /// @brief Updates the editor UI with timestep.
    /// @return Scene extent in the screen.
    Vec2 update(float delta, Vec2 screenExtent);

    void submit_frame();

    static void on_window_event(const WindowEvent* event, void* user);
    static void on_editor_event(const EditorEvent* event, void* user);
    static void on_render(ScreenRenderComponent renderer, void* user);
    static void on_render_dialog(ScreenRenderComponent renderer, void* user);
    static void on_scene_pick(SceneOverlayGizmoID gizmoID, RUID ruid, void* user);

private:
    void main_pre_update();
    void main_update();
    void main_post_update();

    /// @brief Get the main camera used to render scene from.
    Camera get_main_camera();

    /// @brief Get region viewports to render screen contents.
    Vector<RenderSystemScreenPass::Region> get_screen_regions();

private:
    EditorContext mCtx;
    EditorUITopBar mTopBar;
    EditorUIDialog mDialog;
    EditorUIMain mMain;
    EditorUIModal mModal;
    EditorUpdateTick mTick;
    RenderSystem mRenderSystem{};
    RUID mEnvCubemap = 0;
};

} // namespace LD