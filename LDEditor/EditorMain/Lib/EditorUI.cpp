#include "EditorUI.h"
#include <Ludens/Application/Event.h>
#include <Ludens/Profiler/Profiler.h>
#include <LudensEditor/EditorContext/EditorWindowObj.h>

namespace LD {

void EditorUI::startup(const EditorUIInfo& info)
{
    LD_PROFILE_SCOPE;

    mCtx = info.ctx;

    // the WindowManager drives an internal UIF Context and organizes windows
    UIWindowManagerInfo wmI{};
    wmI.fontAtlas = info.fontAtlas;
    wmI.fontAtlasImage = info.fontAtlasImage;
    wmI.screenSize = Vec2((float)info.screenWidth, (float)info.screenHeight);
    wmI.theme = mCtx.get_settings().get_theme().get_ui_theme();
    mWM = UIWindowManager::create(wmI);

    UIWindowAreaID viewportArea = mWM.get_root_area();
    UIWindowAreaID outlinerArea = mWM.split_right(viewportArea, 0.7f);
    UIWindowAreaID inspectorArea = mWM.split_bottom(outlinerArea, 0.5f);

    mTopBar.startup(mWM.get_topbar_window());

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
}

void EditorUI::cleanup()
{
    mTopBar.cleanup();

    EInspectorWindow::destroy(mInspectorWindow);
    EOutlinerWindow::destroy(mOutlinerWindow);
    EViewportWindow::destroy(mViewportWindow);
    UIWindowManager::destroy(mWM);
}

void EditorUI::update(float delta)
{
    mWM.update(delta);
}

void EditorUI::on_render(ScreenRenderComponent renderer, void* user)
{
    EditorUI& self = *(EditorUI*)user;

    // draws the top bar and area windows
    self.mWM.render(renderer);
}

void EditorUI::on_overlay_render(ScreenRenderComponent renderer, void* user)
{
    EditorUI& self = *(EditorUI*)user;

    self.mTopBar.draw_overlay(renderer);

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

void EditorUI::on_event(const Event* event, void* user)
{
    EditorUI& self = *(EditorUI*)user;
    UIContext ctx = self.mWM.get_context();

    switch (event->type)
    {
    case EVENT_TYPE_APPLICAITON_RESIZE:
        self.mWM.resize(Vec2(static_cast<const ApplicationResizeEvent*>(event)->width,
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