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

    mCtx = info.ctx;

    UIContextInfo ctxI{};
    ctxI.fontAtlas = info.fontAtlas;
    ctxI.fontAtlasImage = info.fontAtlasImage;
    ctxI.theme = mCtx.get_theme().get_ui_theme();
    mUI = UIContext::create(ctxI);
    mUIGroundLayer = mUI.create_layer("ground");
    mUIFloatLayer = mUI.create_layer("float");
    mUIModalLayer = mUI.create_layer("modal");

    const Vec2 screenSize((float)info.screenWidth, (float)info.screenHeight);

    EditorTopBarInfo barI{};
    barI.barHeight = EDITOR_BAR_HEIGHT;
    barI.ctx = mCtx;
    barI.floatLayer = mUIFloatLayer;
    barI.groundLayer = mUIGroundLayer;
    barI.screenSize = screenSize;
    mTopBar = EditorTopBar::create(barI);

    // TODO: EditorGroundLayer
    EditorWorkspaceInfo workspaceI{};
    workspaceI.ctx = mCtx;
    workspaceI.layer = mUIGroundLayer;
    workspaceI.rootRect = Rect(0.0f, EDITOR_BAR_HEIGHT, screenSize.x, screenSize.y);
    workspaceI.isVisible = true;
    workspaceI.isFloat = false;
    mSceneWorkspace = EditorWorkspace::create(workspaceI);
    EditorAreaID viewportArea = mSceneWorkspace.get_root_id();
    EditorAreaID outlinerArea = mSceneWorkspace.split_right(viewportArea, 0.7f);
    EditorAreaID inspectorArea = mSceneWorkspace.split_bottom(outlinerArea, 0.5f);
    EditorAreaID consoleArea = mSceneWorkspace.split_bottom(viewportArea, 0.7f);

    mViewportWindow = (ViewportWindow)mSceneWorkspace.create_window(viewportArea, EDITOR_WINDOW_VIEWPORT);
    mOutlinerWindow = (OutlinerWindow)mSceneWorkspace.create_window(outlinerArea, EDITOR_WINDOW_OUTLINER);
    mInspectorWindow = (InspectorWindow)mSceneWorkspace.create_window(inspectorArea, EDITOR_WINDOW_INSPECTOR);
    mConsoleWindow = (ConsoleWindow)mSceneWorkspace.create_window(consoleArea, EDITOR_WINDOW_CONSOLE);
    mConsoleWindow.observe_channel(get_lua_script_log_channel_name());

    // TODO: EditorFloatLayer
    workspaceI.layer = mUIFloatLayer;
    workspaceI.rootRect = Rect(10.0f, 10.0f, 400.0f, 200.0f);
    workspaceI.isVisible = false;
    workspaceI.isFloat = true;
    mFloatWorkspace = EditorWorkspace::create(workspaceI);
    mVersionWindow = (VersionWindow)mFloatWorkspace.create_window(mFloatWorkspace.get_root_id(), EDITOR_WINDOW_VERSION);

    // TODO: EditorModalLayer
    UILayoutInfo layoutI{};
    layoutI.sizeX = UISize::fixed(info.screenWidth);
    layoutI.sizeY = UISize::fixed(info.screenHeight);
    UIWindowInfo windowI{};
    windowI.hidden = true;
    mModalBackdropWorkspace = mUIModalLayer.create_workspace(Rect(0.0f, 0.0f, info.screenWidth, info.screenHeight));
    mModalBackdropWindow = mModalBackdropWorkspace.create_window(mModalBackdropWorkspace.get_root_id(), layoutI, windowI, nullptr);
    mModalBackdropWindow.set_pos(Vec2(0.0f, 0.0f));
    mModalBackdropWindow.set_on_draw([](UIWidget widget, ScreenRenderComponent renderer) {
        renderer.draw_rect(widget.get_rect(), 0x101010C0);
    });

    workspaceI.layer = mUIModalLayer;
    workspaceI.rootRect = Rect(10.0f, 10.0f, 600.0f, 300.0f);
    workspaceI.isVisible = false;
    workspaceI.isFloat = true;
    mModalWorkspace = EditorWorkspace::create(workspaceI);
    mSelectionWindow = (SelectionWindow)mModalWorkspace.create_window(mModalWorkspace.get_root_id(), EDITOR_WINDOW_SELECTION);

    // force window layout
    mUI.update(0.0f);
}

void EditorUI::cleanup()
{
    ui_imgui_release(mUI);

    EditorWorkspace::destroy(mModalWorkspace);
    mModalWorkspace = {};

    EditorWorkspace::destroy(mFloatWorkspace);
    mFloatWorkspace = {};

    EditorWorkspace::destroy(mSceneWorkspace);
    mSceneWorkspace = {};

    EditorTopBar::destroy(mTopBar);
    mTopBar = {};

    UIContext::destroy(mUI);
}

void EditorUI::update(float delta)
{
    LD_PROFILE_SCOPE;

    ui_frame_begin(mUI);
    mTopBar.on_imgui(delta);
    mSceneWorkspace.on_imgui(delta);
    mFloatWorkspace.on_imgui(delta);
    mModalWorkspace.on_imgui(delta);
    ui_frame_end();

    update_ground_workspace();
    update_modal_workspace();

    mUI.update(delta);
}

void EditorUI::resize(const Vec2& screenSize)
{
    // skip minimization
    if (screenSize.x == 0.0f || screenSize.y == 0.0f)
        return;

    // recalculate workspace window areas
    Rect groundRect = Rect(0.0f, EDITOR_BAR_HEIGHT, screenSize.x, screenSize.y - EDITOR_BAR_HEIGHT);
    mSceneWorkspace.set_rect(groundRect);

    Rect screenRect(0.0f, 0.0f, screenSize.x, screenSize.y);
    mModalBackdropWorkspace.set_rect(screenRect);
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

    self.mUIModalLayer.render(renderer);
}

void EditorUI::on_scene_pick(SceneOverlayGizmoID gizmoID, RUID ruid, void* user)
{
    EditorUI& self = *(EditorUI*)user;

    self.mViewportWindow.hover_id(gizmoID, ruid);
}

void EditorUI::modal_open_scene()
{
    LD_ASSERT(mModal == MODAL_NONE);
    mModal = MODAL_OPEN_SCENE;

    mModalWorkspace.set_visible(true);
    mSelectionWindow.show(mCtx.get_project_directory(), "toml");
}

void EditorUI::modal_select_asset(AssetType type)
{
    LD_ASSERT(mModal == MODAL_NONE);
    mModal = MODAL_SELECT_ASSET;

    mModalWorkspace.set_visible(true);
    mSelectionWindow.show(mCtx.get_project_directory(), "lda");
}

void EditorUI::modal_select_script()
{
    LD_ASSERT(mModal == MODAL_NONE);
    mModal = MODAL_SELECT_SCRIPT;

    mModalWorkspace.set_visible(true);
    mSelectionWindow.show(mCtx.get_project_directory(), "lua");
}

void EditorUI::show_version_window()
{
    // TODO: centered
    mFloatWorkspace.set_visible(true);
    mVersionWindow.show();
}

void EditorUI::update_ground_workspace()
{
    AUID oldAssetID = 0;
    AssetType type;

    if (mInspectorWindow.has_component_asset_request(mSubjectCompID, oldAssetID, type))
    {
        modal_select_asset(type);
    }
}

void EditorUI::update_modal_workspace()
{
    FS::Path selectedPath;

    if (mModal == MODAL_NONE)
    {
        mModalBackdropWindow.hide();
        return;
    }

    if (mSelectionWindow.has_canceled())
    {
        mModal = MODAL_NONE;
        mModalWorkspace.set_visible(false);
        return;
    }

    mModalWorkspace.set_visible(true);
    mModalBackdropWindow.show();

    switch (mModal)
    {
    case MODAL_OPEN_SCENE:
        if (mSelectionWindow.has_selected(selectedPath))
        {
            mModal = MODAL_NONE;
            mCtx.action_open_scene(selectedPath);
        }
        break;
    case MODAL_SELECT_ASSET:
        if (mSelectionWindow.has_selected(selectedPath))
        {
            mModal = MODAL_NONE;

            std::string stem = selectedPath.stem().string();
            AssetManager AM = mCtx.get_asset_manager();
            AUID assetID = AM.get_id_from_name(stem.c_str(), nullptr);
            mCtx.action_set_component_asset(mSubjectCompID, assetID);
        }
        break;
    case MODAL_SELECT_SCRIPT:
        if (mSelectionWindow.has_selected(selectedPath))
        {
            if (!mCtx.get_component_base(mSubjectCompID))
                return; // component out of date

            AssetType type;
            AssetManager AM = mCtx.get_asset_manager();
            std::string stem = selectedPath.stem().string();
            AUID scriptAssetID = AM.get_id_from_name(stem.c_str(), &type);
            if (scriptAssetID == 0 || type != ASSET_TYPE_LUA_SCRIPT)
                return; // script asset out of date

            mCtx.action_add_component_script(mSubjectCompID, scriptAssetID);
        }
        break;
    default:
        break;
    }
}

void EditorUI::on_event(const Event* event, void* user)
{
    EditorUI& self = *(EditorUI*)user;

    switch (event->type)
    {
    case EVENT_TYPE_WINDOW_RESIZE:
        self.resize(Vec2(static_cast<const WindowResizeEvent*>(event)->width,
                         static_cast<const WindowResizeEvent*>(event)->height));
        break;
    default:
        self.mUI.on_event(event);
        break;
    }
}

} // namespace LD