#include "EditorUI.h"
#include <Ludens/Application/Event.h>
#include <Ludens/Profiler/Profiler.h>
#include <LudensEditor/EditorContext/EditorWindowObj.h>
#include <LudensEditor/EditorWidget/UIVersionWindow.h>

namespace LD {

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
    mWM = UIWindowManager::create(wmI);

    UIWMAreaID viewportArea = mWM.get_root_area();
    UIWMAreaID outlinerArea = mWM.split_right(viewportArea, 0.7f);
    UIWMAreaID inspectorArea = mWM.split_bottom(outlinerArea, 0.5f);

    // the EditorUI class has an additional Top Bar and Bottom Bar
    EditorTopBarInfo topBarI{};
    topBarI.barHeight = (float)info.barHeight;
    topBarI.context = mWM.get_context();
    topBarI.editorUI = this;
    topBarI.editorTheme = mCtx.get_theme();
    topBarI.screenSize = wmI.screenSize;
    mTopBar.startup(topBarI);

    EditorBottomBarInfo bottomBarI{};
    bottomBarI.barHeight = (float)info.barHeight;
    bottomBarI.context = mWM.get_context();
    bottomBarI.theme = mCtx.get_theme();
    bottomBarI.screenSize = wmI.screenSize;
    mBottomBar.startup(bottomBarI);

    // force window layout
    mWM.update(0.0f);

    {
        EViewportWindowInfo windowI{};
        windowI.ctx = mCtx;
        windowI.areaID = viewportArea;
        windowI.wm = mWM;
        mViewportWindow = EViewportWindow::create(windowI);
    }

    {
        EInspectorWindowInfo windowI{};
        windowI.ctx = mCtx;
        windowI.areaID = inspectorArea;
        windowI.wm = mWM;
        mInspectorWindow = EInspectorWindow::create(windowI);
    }

    {
        EOutlinerWindowInfo windowI{};
        windowI.ctx = mCtx;
        windowI.areaID = outlinerArea;
        windowI.wm = mWM;
        mOutlinerWindow = EOutlinerWindow::create(windowI);
    }

    mVersionWindowID = 0;
}

void EditorUI::cleanup()
{
    mBottomBar.cleanup();
    mTopBar.cleanup();

    if (mVersionWindow)
    {
        UIVersionWindow::destroy(mVersionWindow);
        mVersionWindow = {};
        mVersionWindowID = 0;
    }

    EInspectorWindow::destroy(mInspectorWindow);
    EOutlinerWindow::destroy(mOutlinerWindow);
    EViewportWindow::destroy(mViewportWindow);
    UIWindowManager::destroy(mWM);
}

void EditorUI::update(float delta)
{
    mWM.update(delta);
}

void EditorUI::resize(const Vec2& screenSize)
{
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

    // draw workspace windows
    self.mWM.render(renderer);

    // draw top bar and bottom bar
    UIWindow topbar = self.mTopBar.get_handle();
    topbar.draw(renderer);

    UIWindow bottomBar = self.mBottomBar.get_handle();
    bottomBar.draw(renderer);
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

void EditorUI::show_version_window()
{
    if (mVersionWindowID == 0)
    {
        UIVersionWindowInfo windowI{};
        windowI.context = mWM.get_context();
        windowI.theme = mCtx.get_theme();
        mVersionWindow = UIVersionWindow::create(windowI);

        UIWMClientInfo clientI{};
        clientI.client = mVersionWindow.get_handle();
        clientI.resizeCallback = nullptr;
        clientI.user = this;
        mVersionWindowID = mWM.create_float(clientI);
        mWM.set_close_callback(mVersionWindowID, [](UIWindow client, void* user) {
            EditorUI& self = *(EditorUI*)user;
            self.mVersionWindowID = 0;

            UIVersionWindow::destroy(self.mVersionWindow);
            self.mVersionWindow = {};
        });
    }

    mWM.set_float_pos_centered(mVersionWindowID);
    mWM.show_float(mVersionWindowID);
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
    case EVENT_TYPE_KEY_DOWN:
        ctx.input_key_down(static_cast<const KeyDownEvent*>(event)->key);
        break;
    case EVENT_TYPE_KEY_UP:
        ctx.input_key_up(static_cast<const KeyUpEvent*>(event)->key);
        break;
    case EVENT_TYPE_MOUSE_MOTION:
        ctx.input_mouse_position(Vec2(static_cast<const MouseMotionEvent*>(event)->xpos,
                                      static_cast<const MouseMotionEvent*>(event)->ypos));
        break;
    case EVENT_TYPE_MOUSE_DOWN:
        ctx.input_mouse_down(static_cast<const MouseDownEvent*>(event)->button);
        break;
    case EVENT_TYPE_MOUSE_UP:
        ctx.input_mouse_up(static_cast<const MouseUpEvent*>(event)->button);
        break;
    default:
        break;
    }
}

} // namespace LD