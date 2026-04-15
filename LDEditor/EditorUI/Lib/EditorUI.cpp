#include <Ludens/Event/Event.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Scene/Component/Camera2DView.h>
#include <Ludens/Scene/Component/Sprite2DView.h>
#include <Ludens/Scene/Scene.h>
#include <Ludens/UI/UIImmediate.h>
#include <Ludens/WindowRegistry/WindowRegistry.h>
#include <LudensEditor/AssetImportWindow/AssetImportWindow.h>
#include <LudensEditor/EditorContext/EditorIconAtlas.h>
#include <LudensEditor/EditorContext/EditorWindow.h>
#include <LudensEditor/EditorUI/EditorUI.h>
#include <LudensEditor/EditorWidget/EditorWidget.h>

#include "EditorUIDef.h"

namespace LD {

void EditorUI::startup(const EditorUIInfo& info)
{
    LD_PROFILE_SCOPE;

    LD_ASSERT(info.renderSystem);

    mCtx = info.ctx;
    mCtx.add_observer(&EditorUI::on_editor_event, this);
    mTick = {};
    mTick.screenSize = Vec2((float)info.screenWidth, (float)info.screenHeight);
    mRenderSystem = info.renderSystem;
    mEnvCubemap = (RUID)0; // info.envCubemap

    ui_imgui_startup(mCtx.get_font_default(), mCtx.get_font_mono());
    eui_startup(mCtx);
    eui_push_theme(mCtx.get_theme());

    EditorUITopBarInfo barI{};
    barI.ctx = mCtx;
    barI.layerName = EDITOR_UI_LAYER_TOP_BAR_NAME;
    barI.barHeight = EDITOR_BAR_HEIGHT;
    barI.screenSize = mTick.screenSize;
    mTopBar = EditorUITopBar::create(barI);

    EditorUIMainInfo mainI{};
    mainI.ctx = mCtx;
    mainI.screenSize = mTick.screenSize;
    mainI.topBarHeight = EDITOR_BAR_HEIGHT;
    mMain = EditorUIMain::create(mainI);

    EditorUIModalInfo modalI{};
    modalI.ctx = mCtx;
    modalI.layerName = EDITOR_UI_LAYER_MODAL_NAME;
    modalI.screenSize = mTick.screenSize;
    mModal = EditorUIModal::create(modalI);
    mModal.set_visible(true); // Guard until a project is loaded

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

Vec2 EditorUI::update(float delta, Vec2 screenSize)
{
    LD_PROFILE_SCOPE;

    // skip minimizations
    if (screenSize.x > 0.0f && screenSize.y > 0.0f)
        mTick.screenSize = screenSize;

    mTick.delta = delta;

    main_pre_update();
    main_update();
    main_post_update();

    // Update dialog window
    mDialog.update(delta);

    return mMain.get_viewport_scene_size();
}

// Main window resizes
void EditorUI::main_pre_update()
{
    mTopBar.pre_update(mTick);
    mMain.pre_update(mTick);
    mModal.pre_update(mTick);
}

// Main window imgui pass
void EditorUI::main_update()
{
    CursorType cursorHint;

    eui_begin_window(WindowRegistry::get().get_root_id());
    ui_context_begin(EDITOR_UI_CONTEXT_NAME, mTick.screenSize);
    mTopBar.update(mTick);
    mMain.update(mTick);
    mModal.update(mTick);
    ui_context_end(mTick.delta, cursorHint);
    if (eui_get_window_cursor() == CURSOR_TYPE_DEFAULT)
        eui_set_window_cursor(cursorHint);
    eui_end_window();
}

// Main window deferred destructions
void EditorUI::main_post_update()
{
    mMain.post_update();
    mModal.post_update();
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
    frameI.clearColor = mCtx.get_scene_clear_color();
    mRenderSystem.next_frame(frameI);

    // render ScreenLayer items in regions
    Vector<RenderSystemScreenPass::Region> regions = get_screen_regions();

    RenderSystemScreenPass screenP{};
    screenP.user = this;
    screenP.mat4Callback = &EditorUI::on_mat4_callback;
    screenP.regionCount = regions.size();
    screenP.regions = regions.data();
    screenP.overlay.renderCallback = &EditorUI::on_screen_pass_overlay;
    screenP.overlay.viewport = scene2DViewport;
    mRenderSystem.screen_pass(screenP);

    // render the editor UI
    RenderSystemEditorPass editorP{};
    editorP.user = this;
    editorP.renderCallback = &EditorUI::on_render;
    editorP.scenePickCallback = &EditorUI::on_scene_pick;
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
        editorDP.user = this;
        editorDP.dialogWindow = dialogWindowID;
        editorDP.renderCallback = &EditorUI::on_render_dialog;
        mRenderSystem.editor_dialog_pass(editorDP);
    }

    mRenderSystem.submit_frame();
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

void EditorUI::on_screen_pass_overlay(ScreenRenderComponent renderer, TView<int> regionVPIndices, int overlayVPIndex, void* user)
{
    EditorUI& self = *(EditorUI*)user;
    EditorContext ctx = self.mCtx;
    Scene scene = ctx.get_scene();
    Camera2D editorCamera = self.mMain.get_viewport_camera_2d();

    if (!scene || !editorCamera)
        return;

    // TODO: Editor toggle to display screen UI?
    renderer.bind_quad_pipeline(QUAD_PIPELINE_UBER);
    renderer.set_view_projection_index(overlayVPIndex);
    scene.render_screen_ui(renderer);

    if (ctx.is_playing())
        return;

    Vector<ComponentView> camera2Ds = scene.get_components(COMPONENT_TYPE_CAMERA_2D);
    ComponentView selectedComp = ctx.get_selected_component_view();
    const float thickness = 2.0f / editorCamera.get_zoom();
    Color hightlightColor;
    ctx.get_theme().get_gizmo_highlight_color(hightlightColor);
    Color cameraOutlineColor = 0x00FF00FF; // TODO: source of truth

    for (int i = 0; i < regionVPIndices.size; i++)
    {
        renderer.set_view_projection_index(regionVPIndices.data[i]);

        // Draw selected component outline
        Mat4 worldMat4;
        if (selectedComp && selectedComp.get_world_mat4(worldMat4))
        {

            if (selectedComp.type() == COMPONENT_TYPE_SPRITE_2D)
            {
                Sprite2DView sprite2D(selectedComp);
                Rect rect = Rect::grow(sprite2D.local_rect(), thickness);
                renderer.draw_rect_outline(worldMat4, rect, hightlightColor, thickness);
            }
        }

        // Draw Camera2D outline
        for (size_t i = 0; i < camera2Ds.size(); i++)
        {
            Camera2DView camera2D = (Camera2DView)camera2Ds[i];
            if (!camera2D.get_world_mat4(worldMat4))
                continue;

            Vec2 extent = camera2D.get_extent() / camera2D.get_zoom();
            Rect rect(-extent.x / 2.0f, -extent.y / 2.0f, extent.x, extent.y);
            rect = Rect::grow(rect, thickness);
            renderer.draw_rect_outline(worldMat4, rect, cameraOutlineColor, thickness);
        }
    }
}

bool EditorUI::on_mat4_callback(RUID ruid, Mat4& mat4, void* user)
{
    EditorUI& self = *(EditorUI*)user;

    return EditorContext::render_system_mat4_callback(ruid, mat4, self.mCtx.unwrap());
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
    LD_ASSERT(editorCamera);

    RenderSystemScreenPass::Region region;
    region.viewport = editorCamera.get_viewport();
    region.viewport.region = Rect(0.0f, 0.0f, 1.0f, 1.0f);
    region.worldAABB = editorCamera.get_world_aabb();

    return {region};
}

void EditorUI::on_window_event(const WindowEvent* event, void* user)
{
    EditorUI& self = *(EditorUI*)user;

    ui_context_input(EDITOR_UI_CONTEXT_NAME, event);
}

void EditorUI::on_editor_event(const EditorEvent* event, void* user)
{
    // This is called from EditorContext::poll_events,
    // we can emit new events but synchronous calls
    // should be deferred to next imgui update.
    EditorUI& self = *(EditorUI*)user;

    AssetImportWindow importWindow{};

    switch (event->type)
    {
    case EDITOR_EVENT_TYPE_REQUEST_NEW_PROJECT:
        self.mModal.set_window(EDITOR_WINDOW_PROJECT);
        self.mModal.set_visible(true);
        break;
    case EDITOR_EVENT_TYPE_REQUEST_WORKSPACE_LAYOUT:
    {
        auto* requestE = (const EditorRequestWorkspaceLayoutEvent*)event;
        self.mMain.set_layout(requestE->layout);
        break;
    }
    case EDITOR_EVENT_TYPE_REQUEST_IMPORT_ASSETS:
    {
        auto* requestE = (const EditorRequestImportAssetsEvent*)event;
        importWindow = (AssetImportWindow)self.mModal.set_window(EDITOR_WINDOW_ASSET_IMPORT);
        importWindow.set_type(ASSET_TYPE_TEXTURE_2D);
        importWindow.set_source_path(requestE->srcPath.string());
        self.mModal.set_visible(true);
        break;
    }
    case EDITOR_EVENT_TYPE_NOTIFY_PROJECT_CREATION:
    {
        auto* notifyE = (const EditorNotifyProjectCreationEvent*)event;
        if (notifyE->error.empty())
        {
            auto* actionE = (EditorActionOpenProjectEvent*)self.mCtx.enqueue_event(EDITOR_EVENT_TYPE_ACTION_OPEN_PROJECT);
            actionE->projectSchema = notifyE->projectSchema;
        }
        break;
    }
    case EDITOR_EVENT_TYPE_NOTIFY_PROJECT_LOAD:
        self.mModal.set_visible(false);
        break;
    default:
        break;
    }
}

} // namespace LD
