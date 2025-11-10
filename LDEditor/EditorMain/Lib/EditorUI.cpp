#include "EditorUI.h"
#include <Ludens/Application/Application.h>
#include <Ludens/Application/Event.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Scene/Scene.h>
#include <Ludens/UI/UIImmediate.h>
#include <LudensEditor/EditorContext/EditorIconAtlas.h>
#include <LudensEditor/EditorContext/EditorWindowObj.h>
#include <LudensEditor/EditorWidget/UIVersionWindow.h>

namespace LD {

constexpr Hash32 sUIGroundLayerHash("UIGroundLayer");
constexpr Hash32 sUIFloatLayerHash("UIFloatLayer");

void EditorUI::startup(const EditorUIInfo& info)
{
    LD_PROFILE_SCOPE;

    mCtx = info.ctx;

    // the WindowManager drives an internal UI Context and organizes windows
    UIWindowManagerInfo wmI{};
    wmI.fontAtlas = info.fontAtlas;
    wmI.fontAtlasImage = info.fontAtlasImage;
    wmI.screenSize = Vec2((float)info.screenWidth, (float)info.screenHeight);
    wmI.theme = mCtx.get_settings().get_theme().get_ui_theme();
    wmI.topBarHeight = info.barHeight;
    wmI.bottomBarHeight = info.barHeight;
    wmI.iconAtlasImage = mCtx.get_editor_icon_atlas();
    wmI.icons.close = EditorIconAtlas::get_icon_rect(EditorIcon::Close);
    wmI.groundLayerHash = sUIGroundLayerHash;
    wmI.floatLayerHash = sUIFloatLayerHash;
    mWM = UIWindowManager::create(wmI);

    UIWMAreaID viewportArea = mWM.get_root_area();
    UIWMAreaID outlinerArea = mWM.split_right(viewportArea, 0.7f);
    UIWMAreaID inspectorArea = mWM.split_bottom(outlinerArea, 0.5f);
    UIWMAreaID consoleArea = mWM.split_bottom(viewportArea, 0.7f);
    UIContext uiCtx = mWM.get_context();

    UILayoutInfo layoutI{};
    layoutI.sizeX = UISize::fixed(info.screenWidth);
    layoutI.sizeY = UISize::fixed(info.screenHeight);
    UIWindowInfo windowI{};
    windowI.name = "backdrop";
    windowI.hidden = true;
    windowI.layer = sUIFloatLayerHash;
    mBackdropWindow = uiCtx.add_window(layoutI, windowI, this);
    mBackdropWindow.set_pos(Vec2(0.0f, 0.0f));
    mBackdropWindow.set_on_draw([](UIWidget widget, ScreenRenderComponent renderer) {
        renderer.draw_rect(widget.get_rect(), 0x101010C0);
    });

    // the EditorUI class has an additional Top Bar and Bottom Bar
    EditorTopBarInfo topBarI{};
    topBarI.barHeight = (float)info.barHeight;
    topBarI.context = uiCtx;
    topBarI.editorUI = this;
    topBarI.editorTheme = mCtx.get_theme();
    topBarI.screenSize = wmI.screenSize;
    topBarI.layer = sUIGroundLayerHash;
    mTopBar.startup(topBarI);

    EditorBottomBarInfo bottomBarI{};
    bottomBarI.barHeight = (float)info.barHeight;
    bottomBarI.context = mWM.get_context();
    bottomBarI.theme = mCtx.get_theme();
    bottomBarI.screenSize = wmI.screenSize;
    bottomBarI.layer = sUIGroundLayerHash;
    mBottomBar.startup(bottomBarI);

    // force window layout
    mWM.update(0.0f);

    mEditorWindows.clear();

    {
        EViewportWindowInfo windowI{};
        windowI.ctx = mCtx;
        windowI.areaID = viewportArea;
        windowI.wm = mWM;
        mViewportWindow = EViewportWindow::create(windowI);
        mEditorWindows.push_back((EditorWindowObj*)mViewportWindow.unwrap());
    }

    {
        EInspectorWindowInfo windowI{};
        windowI.ctx = mCtx;
        windowI.areaID = inspectorArea;
        windowI.wm = mWM;
        windowI.selectAssetFn = &EditorUI::ECB::select_asset;
        windowI.user = this;
        mInspectorWindow = EInspectorWindow::create(windowI);
        mEditorWindows.push_back((EditorWindowObj*)mInspectorWindow.unwrap());
    }

    {
        EOutlinerWindowInfo windowI{};
        windowI.ctx = mCtx;
        windowI.areaID = outlinerArea;
        windowI.wm = mWM;
        windowI.addScriptToComponent = &EditorUI::ECB::add_script_to_component;
        windowI.user = this;
        mOutlinerWindow = EOutlinerWindow::create(windowI);
        mEditorWindows.push_back((EditorWindowObj*)mOutlinerWindow.unwrap());
    }

    {
        EConsoleWindowInfo windowI{};
        windowI.ctx = mCtx;
        windowI.areaID = consoleArea;
        windowI.wm = mWM;
        windowI.user = this;
        mConsoleWindow = EConsoleWindow::create(windowI);
        mConsoleWindow.observe_channel(get_lua_script_log_channel_name());
        mEditorWindows.push_back((EditorWindowObj*)mConsoleWindow.unwrap());
    }
}

void EditorUI::cleanup()
{
    ui_imgui_release(mWM.get_context());

    mBottomBar.cleanup();
    mTopBar.cleanup();

    if (mVersionWindowID && mVersionWindow)
    {
        UIVersionWindow::destroy(mVersionWindow);
        mVersionWindow = {};
        mVersionWindowID = 0;
    }

    EConsoleWindow::destroy(mConsoleWindow);
    EInspectorWindow::destroy(mInspectorWindow);
    EOutlinerWindow::destroy(mOutlinerWindow);
    EViewportWindow::destroy(mViewportWindow);
    UIWindowManager::destroy(mWM);
}

void EditorUI::update(float delta)
{
    LD_PROFILE_SCOPE;

    Application app = Application::get();

    ui_frame_begin(mWM.get_context());

    for (EditorWindowObj* window : mEditorWindows)
    {
        window->on_imgui();
    }

    bool hasBackdrop = mSelectWindow.isActive;
    if (hasBackdrop)
    {
        ui_push_window("Backdrop", mBackdropWindow);
        ui_set_window_rect(Rect(0.0f, 0.0f, app.width(), app.height()));
        ui_pop_window();
    }
    else
        mBackdropWindow.hide();

    if (mSelectWindow.isActive)
    {
        FS::Path selectedPath;
        if (eui_select_window(&mSelectWindow, selectedPath) && mSelectWindow.onSelect)
            mSelectWindow.onSelect(selectedPath, mSelectWindow.user);

        if (!mSelectWindow.isActive)
            mWM.hide_float(mSelectWindowID);
    }

    ui_frame_end();

    mWM.update(delta);
}

void EditorUI::resize(const Vec2& screenSize)
{
    // skip minimization
    if (screenSize.x == 0.0f || screenSize.y == 0.0f)
        return;

    // resize top bar
    UIWindow topbar = mTopBar.get_handle();
    float barHeight = topbar.get_size().y;
    topbar.set_size(Vec2(screenSize.x, barHeight));

    // resize bottom bar
    UIWindow bottomBar = mBottomBar.get_handle();
    barHeight = bottomBar.get_size().y;
    bottomBar.set_size(Vec2(screenSize.x, barHeight));
    bottomBar.set_pos(Vec2(0.0f, screenSize.y - barHeight));

    // recalculate workspace window areas
    mWM.resize(screenSize);
}

void EditorUI::on_render(ScreenRenderComponent renderer, void* user)
{
    EditorUI& self = *(EditorUI*)user;

    UIContext uiCtx = self.mWM.get_context();

    uiCtx.render_layer(sUIGroundLayerHash, renderer);
    uiCtx.render_layer(sUIFloatLayerHash, renderer);

    /*
    // draw workspace windows
    self.mWM.render_workspace(renderer);

    // draw top bar and bottom bar
    UIWindow topbar = self.mTopBar.get_handle();
    topbar.draw(renderer);

    UIWindow bottomBar = self.mBottomBar.get_handle();
    bottomBar.draw(renderer);

    // draw backdrop window
    self.mBackdropWindow.draw(renderer);

    // draw floating windows
    self.mWM.render_float(renderer);
    */
}

void EditorUI::on_overlay_render(ScreenRenderComponent renderer, void* user)
{
    EditorUI& self = *(EditorUI*)user;

    // NOTE: The UIWindowManager is not aware of the overlay render pass.
    //       Here we explicitly call draw-overlay functions on windows
    //       via runtime polymorphism.

    std::vector<UIWindow> windows;
    self.mWM.get_workspace_windows(windows);

    for (UIWindow window : windows)
    {
        EditorWindowObj* base = (EditorWindowObj*)window.get_user();
        base->on_draw_overlay(renderer);
    }
}

void EditorUI::on_scene_pick(SceneOverlayGizmoID gizmoID, RUID ruid, void* user)
{
    EditorUI& self = *(EditorUI*)user;

    self.mViewportWindow.hover_id(gizmoID, ruid);
}

void EditorUI::ECB::select_asset(AssetType type, AUID currentID, void* user)
{
    EditorUI& self = *(EditorUI*)user;

    SelectWindowUsage usage{.user = user};
    usage.onSelect = [](const FS::Path& path, void* user) {
        EditorUI& self = *(EditorUI*)user;
        EditorContext ctx = self.mCtx;

        self.mWM.hide_float(self.mSelectWindowID);

        std::string stem = path.stem().string();
        const char* assetName = stem.c_str();
        AssetManager AM = ctx.get_asset_manager();
        AUID assetID = AM.get_id_from_name(assetName, nullptr);

        if (assetID != 0) // TODO:
            self.mInspectorWindow.select_asset(assetID);
    };
    usage.extensionFilter = "ldb";
    usage.directoryPath = self.mCtx.get_project_directory();

    self.show_select_window(usage);
}

void EditorUI::ECB::add_script_to_component(CUID compID, void* user)
{
    EditorUI& self = *(EditorUI*)user;
    self.mState.compID = compID;

    SelectWindowUsage usage{.user = user};
    usage.onSelect = [](const FS::Path& path, void* user) {
        EditorUI& self = *(EditorUI*)user;
        EditorContext ctx = self.mCtx;
        CUID compID = self.mState.compID;
        self.mState.compID = 0;

        self.mWM.hide_float(self.mSelectWindowID);

        if (!ctx.get_component_base(compID))
            return; // component out of date

        AssetType type;
        AssetManager AM = ctx.get_asset_manager();
        std::string stem = path.stem().string();
        AUID scriptAssetID = AM.get_id_from_name(stem.c_str(), &type);
        if (scriptAssetID == 0 || type != ASSET_TYPE_LUA_SCRIPT)
            return; // script asset out of date

        ctx.action_add_component_script(compID, scriptAssetID);
    };
    usage.extensionFilter = "lua";
    usage.directoryPath = self.mCtx.get_project_directory();

    self.show_select_window(usage);
}

void EditorUI::open_scene()
{
    SelectWindowUsage usage{.user = this};
    usage.onSelect = [](const FS::Path& path, void* user) {
        EditorUI& self = *(EditorUI*)user;
        EditorContext ctx = self.mCtx;

        self.mWM.hide_float(self.mSelectWindowID);

        ctx.action_open_scene(path);
    };
    usage.extensionFilter = "toml";
    usage.directoryPath = mCtx.get_project_directory();

    show_select_window(usage);
}

void EditorUI::show_version_window()
{
    if (mVersionWindowID == 0)
    {
        if (!mVersionWindow)
        {
            UIVersionWindowInfo windowI{};
            windowI.context = mWM.get_context();
            windowI.layer = mWM.get_float_layer_hash();
            windowI.theme = mCtx.get_theme();
            mVersionWindow = UIVersionWindow::create(windowI);
        }

        UIWMClientInfo clientI{};
        clientI.client = mVersionWindow.get_handle();
        clientI.user = this;
        mVersionWindowID = mWM.create_float(clientI);
        mWM.set_close_callback(mVersionWindowID, [](UIWindow client, void* user) {
            EditorUI& self = *(EditorUI*)user;
            self.mVersionWindowID = 0;
        });
    }

    mWM.set_float_pos_centered(mVersionWindowID);
    mWM.show_float(mVersionWindowID);
}

void EditorUI::show_select_window(const SelectWindowUsage& usage)
{
    EditorTheme editorTheme = mCtx.get_theme();
    float pad = editorTheme.get_padding();

    if (!mSelectWindowID)
    {
        UILayoutInfo layoutI{};
        layoutI.sizeX = UISize::fixed(600.0f);
        layoutI.sizeY = UISize::fixed(300.0f);
        layoutI.childAxis = UI_AXIS_Y;
        layoutI.childPadding = {pad, pad, pad, pad};
        UIWindowInfo windowI{};
        windowI.name = "Select";
        windowI.layer = sUIFloatLayerHash;
        UIWMClientInfo clientI{};
        clientI.user = this;
        clientI.client = mWM.get_context().add_window(layoutI, windowI, this);
        clientI.client.layout();
        mSelectWindowID = mWM.create_float(clientI);
        mWM.set_close_callback(mSelectWindowID, [](UIWindow client, void* user) {
            EditorUI& self = *(EditorUI*)user;
            self.mSelectWindowID = 0;

            self.mWM.get_context().remove_window(self.mSelectWindow.client);
            self.mSelectWindow.client = {};
            self.mSelectWindow.isActive = false;
        });
    }

    mSelectWindow.client = mWM.get_area_window(mSelectWindowID);
    mSelectWindow.clientName = "Select";
    mSelectWindow.isActive = true;
    mSelectWindow.theme = mCtx.get_theme();
    mSelectWindow.extensionFilter = usage.extensionFilter;
    mSelectWindow.editorIconAtlas = mCtx.get_editor_icon_atlas();
    mSelectWindow.directoryPath = usage.directoryPath;
    mSelectWindow.directoryContents = {};
    mSelectWindow.onSelect = usage.onSelect;
    mSelectWindow.user = usage.user;

    mWM.set_float_pos_centered(mSelectWindowID);
    mWM.show_float(mSelectWindowID);
}

void EditorUI::on_event(const Event* event, void* user)
{
    EditorUI& self = *(EditorUI*)user;
    UIContext ctx = self.mWM.get_context();

    switch (event->type)
    {
    case EVENT_TYPE_APPLICAITON_RESIZE:
        self.resize(Vec2(static_cast<const ApplicationResizeEvent*>(event)->width,
                         static_cast<const ApplicationResizeEvent*>(event)->height));
        break;
    default:
        ctx.forward_event(event);
        break;
    }
}

} // namespace LD