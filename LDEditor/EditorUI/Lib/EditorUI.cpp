#include <Ludens/Event/Event.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Scene/Scene.h>
#include <Ludens/UI/UIImmediate.h>
#include <Ludens/WindowRegistry/WindowRegistry.h>
#include <LudensEditor/EditorContext/EditorIconAtlas.h>
#include <LudensEditor/EditorContext/EditorWindow.h>
#include <LudensEditor/EditorUI/EditorUI.h>
#include <LudensEditor/EditorWidget/EditorWidget.h>

#define EDITOR_BAR_HEIGHT 22.0f
#define EDITOR_UI_CONTEXT_NAME "EDITOR_UI"
#define EDITOR_UI_LAYER_TOP_BAR_NAME "EDITOR_UI_LAYER_TOP_BAR"
#define EDITOR_UI_LAYER_MAIN_NAME "EDITOR_UI_LAYER_MAIN"
#define EDITOR_UI_LAYER_MODAL_NAME "EDITOR_UI_LAYER_MODAL"

namespace LD {

void EditorUI::startup(const EditorUIInfo& info)
{
    LD_PROFILE_SCOPE;

    LD_ASSERT(info.renderSystem);

    mScreenSize = Vec2((float)info.screenWidth, (float)info.screenHeight);
    mCtx = info.ctx;
    mRenderSystem = info.renderSystem;
    mEnvCubemap = (RUID)0; // info.envCubemap

    ui_imgui_startup(mCtx.get_font_default());
    eui_startup(mCtx);
    eui_push_theme(mCtx.get_theme());

    EditorUITopBarInfo barI{};
    barI.ctx = mCtx;
    barI.layerName = EDITOR_UI_LAYER_TOP_BAR_NAME;
    barI.barHeight = EDITOR_BAR_HEIGHT;
    barI.screenSize = mScreenSize;
    mTopBar = EditorUITopBar::create(barI);

    EditorUIMainInfo mainI{};
    mainI.ctx = mCtx;
    mainI.layerName = EDITOR_UI_LAYER_MAIN_NAME;
    mainI.screenSize = mScreenSize;
    mainI.topBarHeight = EDITOR_BAR_HEIGHT;
    mMain = EditorUIMain::create(mainI);

    EditorUIModalInfo modalI{};
    modalI.ctx = mCtx;
    modalI.layerName = EDITOR_UI_LAYER_MODAL_NAME;
    modalI.screenSize = mScreenSize;
    mModal = EditorUIModal::create(modalI);

    EditorUIDialogInfo dialogI{};
    dialogI.ctx = mCtx;
    mDialog = EditorUIDialog::create(dialogI);
}

void EditorUI::cleanup()
{
    LD_PROFILE_SCOPE;

    EditorUIDialog::destroy(mDialog);
    mDialog = {};

    EditorUIModal::destroy(mModal);
    mModal = {};

    EditorUIMain::destroy(mMain);
    mMain = {};

    EditorUITopBar::destroy(mTopBar);
    mTopBar = {};

    eui_pop_theme();
    eui_cleanup();
    ui_imgui_cleanup();
}

void EditorUI::update(float delta)
{
    LD_PROFILE_SCOPE;

    // imgui pass
    ui_context_begin(EDITOR_UI_CONTEXT_NAME, mScreenSize);
    mTopBar.on_imgui(delta);
    mMain.on_imgui(delta);
    mModal.on_imgui(delta, mScreenSize);
    ui_context_end(delta);

    // post imgui update
    mDialog.update(delta);

    // EditorContext update.
    // If the Scene is playing in editor, this drives the Scene update as well
    mCtx.update(mMain.get_viewport_scene_size(), delta);
}

void EditorUI::submit_frame()
{
    LD_PROFILE_SCOPE;

    WindowRegistry reg = WindowRegistry::get();
    const WindowID dialogWindowID = mDialog.get_dialog_window_id();
    const Vec2 windowExtent = reg.get_window_extent(reg.get_root_id());
    const Vec2 sceneExtent = mMain.get_viewport_scene_size();
    const Viewport scene2DViewport = Viewport::from_extent(sceneExtent);
    const Viewport window2DViewport = Viewport::from_extent(windowExtent);

    // begin rendering a frame
    RenderSystemFrameInfo frameI{};
    frameI.directionalLight = Vec3(0.0f, 1.0f, 0.0f);
    frameI.screenExtent = windowExtent;
    frameI.sceneExtent = sceneExtent;
    frameI.envCubemap = mEnvCubemap;
    frameI.dialogWindowID = dialogWindowID;
    frameI.clearColor = mCtx.get_project_settings().get_rendering_settings().get_clear_color();
    mRenderSystem.next_frame(frameI);

    // render ScreenLayer items in regions
    Vector<RenderSystemScreenPass::Region> regions = get_screen_regions();

    RenderSystemScreenPass screenP{};
    screenP.mat4Callback = &EditorContext::render_system_mat4_callback;
    screenP.regionCount = regions.size();
    screenP.regions = regions.data();
    screenP.user = mCtx.unwrap();
    screenP.overlay.renderCallback = &EditorContext::render_system_screen_pass_overlay_callback;
    screenP.overlay.viewport = scene2DViewport;
    mRenderSystem.screen_pass(screenP);

    // render the editor UI
    RenderSystemEditorPass editorP{};
    editorP.renderCallback = &EditorUI::on_render;
    editorP.scenePickCallback = &EditorUI::on_scene_pick;
    editorP.user = this;
    editorP.sceneMousePickQuery = nullptr;
    editorP.viewport = window2DViewport;
    Vec2 queryPos;
    if (mMain.get_viewport_mouse_pos(queryPos))
        editorP.sceneMousePickQuery = &queryPos;
    mRenderSystem.editor_pass(editorP);

    // render dialog window
    if (dialogWindowID)
    {
        RenderSystemEditorDialogPass editorDP{};
        editorDP.dialogWindow = dialogWindowID;
        editorDP.renderCallback = &EditorUI::on_render_dialog;
        editorDP.user = this;
        mRenderSystem.editor_dialog_pass(editorDP);
    }

    mRenderSystem.submit_frame();
}

void EditorUI::resize(const Vec2& screenSize)
{
    // skip minimization
    if (screenSize.x == 0.0f || screenSize.y == 0.0f)
        return;

    mScreenSize = screenSize;

    // recalculate workspace window areas
    mMain.resize(mScreenSize);
}

void EditorUI::on_render(ScreenRenderComponent renderer, void* user)
{
    EditorUI& self = *(EditorUI*)user;

    ui_context_render(EDITOR_UI_CONTEXT_NAME, renderer);
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

Vector<RenderSystemScreenPass::Region> EditorUI::get_screen_regions()
{
    if (mCtx.is_playing()) // in-game screen regions
        return mCtx.get_scene_screen_regions();

    Camera2D editorCamera = mMain.get_viewport_camera_2d();
    RenderSystemScreenPass::Region region;
    region.viewport = editorCamera.get_viewport();
    region.viewport.region = Rect(0.0f, 0.0f, 1.0f, 1.0f);
    region.worldAABB = editorCamera.get_world_aabb();

    return {region};
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

    ui_context_input(EDITOR_UI_CONTEXT_NAME, event);
}

} // namespace LD