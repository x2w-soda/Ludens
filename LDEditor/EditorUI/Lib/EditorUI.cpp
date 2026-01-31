#include <Ludens/Event/Event.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Scene/Scene.h>
#include <Ludens/UI/UIImmediate.h>
#include <Ludens/WindowRegistry/WindowRegistry.h>
#include <LudensEditor/EditorContext/EditorIconAtlas.h>
#include <LudensEditor/EditorContext/EditorWindow.h>
#include <LudensEditor/EditorUI/EditorUI.h>

#define EDITOR_BAR_HEIGHT 22.0f

namespace LD {

void EditorUI::startup(const EditorUIInfo& info)
{
    LD_PROFILE_SCOPE;

    LD_ASSERT(info.fontAtlas);
    LD_ASSERT(info.fontAtlasImage);
    LD_ASSERT(info.renderServer);
    LD_ASSERT(info.envCubemap);

    mCtx = info.ctx;
    mRenderServer = info.renderServer;
    mEnvCubemap = info.envCubemap;

    UIContextInfo ctxI{};
    ctxI.fontAtlas = mFontAtlas = info.fontAtlas;
    ctxI.fontAtlasImage = mFontAtlasImage = info.fontAtlasImage;
    ctxI.theme = mCtx.get_theme().get_ui_theme();
    mUI = UIContext::create(ctxI);
    mUIGroundLayer = mUI.create_layer("ground");
    mUIFloatLayer = mUI.create_layer("float");

    const Vec2 screenSize((float)info.screenWidth, (float)info.screenHeight);

    EditorUITopBarInfo barI{};
    barI.barHeight = EDITOR_BAR_HEIGHT;
    barI.ctx = mCtx;
    barI.floatLayer = mUIFloatLayer;
    barI.groundLayer = mUIGroundLayer;
    barI.screenSize = screenSize;
    mTopBar = EditorUITopBar::create(barI);

    EditorUIMainInfo mainI{};
    mainI.ctx = mCtx;
    mainI.groundLayer = mUIGroundLayer;
    mainI.screenSize = screenSize;
    mainI.topBarHeight = EDITOR_BAR_HEIGHT;
    mMain = EditorUIMain::create(mainI);

    EditorUIDialogInfo dialogI{};
    dialogI.ctx = mCtx;
    dialogI.fontAtlas = mFontAtlas;
    dialogI.fontAtlasImage = mFontAtlasImage;
    mDialog = EditorUIDialog::create(dialogI);

    // TODO: EditorFloatLayer
    /*
    EditorWorkspaceInfo workspaceI{};
    workspaceI.layer = mUIFloatLayer;
    workspaceI.rootRect = Rect(10.0f, 10.0f, 400.0f, 200.0f);
    workspaceI.isVisible = false;
    workspaceI.isFloat = true;
    mFloatWorkspace = EditorWorkspace::create(workspaceI);
    */

    // force window layout
    mUI.update(0.0f);
}

void EditorUI::cleanup()
{
    LD_PROFILE_SCOPE;

    ui_imgui_release(mUI);

    EditorUIDialog::destroy(mDialog);
    mDialog = {};

    EditorUIMain::destroy(mMain);
    mMain = {};

    EditorUITopBar::destroy(mTopBar);
    mTopBar = {};

    UIContext::destroy(mUI);
}

void EditorUI::update(float delta)
{
    LD_PROFILE_SCOPE;

    // imgui pass
    ui_frame_begin(mUI);
    mTopBar.on_imgui(delta);
    mMain.on_imgui(delta);
    ui_frame_end();

    // post imgui update
    mMain.update(delta);
    mDialog.update(delta);

    // Editor UIContext update
    mUI.update(delta);

    // EditorContext update.
    // If the Scene is playing in editor, this drives the Scene update as well
    mCtx.update(mMain.get_viewport_scene_size(), delta);
}

void EditorUI::submit_frame()
{
    LD_PROFILE_SCOPE;

    // If the Scene is playing, the main camera is from some camera component registered in scene.
    // Otherwise it's just the viewport camera.
    Camera mainCamera = get_main_camera();
    LD_ASSERT(mainCamera);

    WindowRegistry reg = WindowRegistry::get();
    const WindowID dialogWindowID = mDialog.get_dialog_window_id();
    const Vec2 screenExtent = reg.get_window_extent(reg.get_root_id());

    // begin rendering a frame
    RenderServerFrameInfo frameI{};
    frameI.directionalLight = Vec3(0.0f, 1.0f, 0.0f);
    frameI.mainCamera = mainCamera;
    frameI.screenExtent = screenExtent;
    frameI.sceneExtent = mMain.get_viewport_scene_size();
    frameI.envCubemap = mEnvCubemap;
    frameI.dialogWindowID = dialogWindowID;
    frameI.clearColor = mCtx.get_project_settings().get_rendering_settings().get_clear_color();
    mRenderServer.next_frame(frameI);

    // render game scene with overlay, the editor context is responsible for supplying object transforms
    RenderServerScenePass sceneP{};
    sceneP.transformCallback = &EditorContext::render_server_transform_callback;
    sceneP.user = mCtx.unwrap();
    sceneP.overlay.enabled = !mCtx.is_playing();
    sceneP.overlay.outlineRUID = mMain.get_viewport_outline_ruid();
    sceneP.hasSkybox = (mEnvCubemap != 0);
    mMain.get_viewport_gizmo_state(
        sceneP.overlay.gizmoType,
        sceneP.overlay.gizmoCenter,
        sceneP.overlay.gizmoScale,
        sceneP.overlay.gizmoColor);
    mRenderServer.scene_pass(sceneP);

    // render screen space items on top of game scene.
    RenderServerScreenPass screenP{};
    screenP.layerCallback = &EditorContext::render_server_screen_pass_callback;
    screenP.user = mCtx.unwrap();
    mRenderServer.screen_pass(screenP);

    // render the editor UI
    RenderServerEditorPass editorP{};
    editorP.renderCallback = &EditorUI::on_render;
    editorP.scenePickCallback = &EditorUI::on_scene_pick;
    editorP.user = this;
    editorP.sceneMousePickQuery = nullptr;
    Vec2 queryPos;
    if (mMain.get_viewport_mouse_pos(queryPos))
        editorP.sceneMousePickQuery = &queryPos;
    mRenderServer.editor_pass(editorP);

    // render the editor overlay UI
    /*
    RenderServerEditorOverlayPass editorOP{};
    editorOP.renderCallback = &EditorUI::on_render_overlay;
    editorOP.blurMixColor = 0x101010FF;
    editorOP.blurMixFactor = 0.1f;
    editorOP.user = this;
    mRenderServer.editor_overlay_pass(editorOP);
    */

    // render dialog window
    if (dialogWindowID)
    {
        RenderServerEditorDialogPass editorDP{};
        editorDP.dialogWindow = dialogWindowID;
        editorDP.renderCallback = &EditorUI::on_render_dialog;
        editorDP.user = this;
        mRenderServer.editor_dialog_pass(editorDP);
    }

    mRenderServer.submit_frame();
}

void EditorUI::resize(const Vec2& screenSize)
{
    // skip minimization
    if (screenSize.x == 0.0f || screenSize.y == 0.0f)
        return;

    // recalculate workspace window areas
    mMain.resize(screenSize);
}

void EditorUI::on_render(ScreenRenderComponent renderer, void* user)
{
    EditorUI& self = *(EditorUI*)user;

    self.mUIGroundLayer.render(renderer);
    self.mUIFloatLayer.render(renderer);
}

void EditorUI::on_render_overlay(ScreenRenderComponent renderer, void* user)
{
    EditorUI& self = *(EditorUI*)user;

    // TODO: self.mUIModalLayer.render(renderer);
}

void EditorUI::on_render_dialog(ScreenRenderComponent renderer, void* user)
{
    EditorUI& self = *(EditorUI*)user;

    self.mDialog.render(renderer);
}

void EditorUI::on_scene_pick(SceneOverlayGizmoID gizmoID, RUID ruid, void* user)
{
    EditorUI& self = *(EditorUI*)user;

    self.mMain.set_viewport_hover_id(gizmoID, ruid);
}

Camera EditorUI::get_main_camera()
{
    Camera sceneCamera;
    if (mCtx.is_playing() && (sceneCamera = mCtx.get_scene_camera()))
        return sceneCamera;

    return mMain.get_viewport_camera();
}

void EditorUI::on_event(const WindowEvent* event, void* user)
{
    EditorUI& self = *(EditorUI*)user;

    switch (event->type)
    {
    case EVENT_TYPE_WINDOW_RESIZE:
        self.resize(Vec2(static_cast<const WindowResizeEvent*>(event)->width,
                         static_cast<const WindowResizeEvent*>(event)->height));
        break;
    default:
        break;
    }

    self.mUI.on_window_event((const WindowEvent*)event);
}

} // namespace LD