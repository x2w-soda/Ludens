#pragma once

#include <Ludens/DSA/Vector.h>
#include <Ludens/Media/Font.h>
#include <Ludens/RenderComponent/SceneOverlayComponent.h>
#include <Ludens/System/FileSystem.h>
#include <Ludens/UI/UIContext.h>
#include <Ludens/UI/UIWindow.h>

#include <LudensEditor/EditorContext/EditorContext.h>

#include <LudensEditor/EditorUI/EditorDialog.h>
#include <LudensEditor/EditorUI/EditorUIDialog.h>
#include <LudensEditor/EditorUI/EditorUIMain.h>
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
    FontAtlas fontAtlas;
    RImage fontAtlasImage;
    RenderServer renderServer;
    RUID envCubemap;
};

/// @brief Editor user interface.
///        Owner of the UIContext for all editor UI.
///        User of the RenderServer to render both game scene and editor.
class EditorUI
{
public:
    /// @brief In-place startup of the editor UI
    void startup(const EditorUIInfo& info);

    /// @brief In-place cleanup of the editor UI
    void cleanup();

    /// @brief Updates the editor UI with timestep.
    void update(float delta);

    void submit_frame();

    /// @brief Resize the editor UI to new screen size.
    void resize(const Vec2& screenSize);

    static void on_event(const WindowEvent* event, void* user);
    static void on_render(ScreenRenderComponent renderer, void* user);
    static void on_render_overlay(ScreenRenderComponent renderer, void* user);
    static void on_render_dialog(ScreenRenderComponent renderer, void* user);
    static void on_scene_pick(SceneOverlayGizmoID gizmoID, RUID ruid, void* user);

private:
    /// @brief Get the main camera used to render scene from.
    Camera get_main_camera();

private:
    EditorContext mCtx;
    EditorUITopBar mTopBar;
    EditorUIDialog mDialog;
    EditorUIMain mMain;
    UIContext mUI{};
    UILayer mUIGroundLayer{};
    UILayer mUIFloatLayer{};
    FontAtlas mFontAtlas{};
    RImage mFontAtlasImage{};
    RenderServer mRenderServer{};
    RUID mEnvCubemap = 0;
};

} // namespace LD