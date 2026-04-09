#include <Ludens/Memory/Memory.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Scene/Scene.h>
#include <LudensEditor/ConsoleWindow/ConsoleWindow.h>
#include <LudensEditor/DocumentWindow/DocumentWindow.h>
#include <LudensEditor/InspectorWindow/InspectorWindow.h>
#include <LudensEditor/OutlinerWindow/OutlinerWindow.h>
#include <LudensEditor/SelectionWindow/SelectionWindow.h>
#include <LudensEditor/VersionWindow/VersionWindow.h>
#include <LudensEditor/ViewportWindow/ViewportWindow.h>

#include <LudensEditor/EditorUI/EditorUIMain.h>
#include <LudensEditor/EditorUI/EditorWorkspace.h>

#include "EditorUIDef.h"

namespace LD {

class EditorUIMainObj
{
    friend class EditorUIMain;

public:
    EditorUIMainObj(const EditorUIMainInfo& mainI);
    EditorUIMainObj(const EditorUIMainObj&) = delete;
    ~EditorUIMainObj();

    EditorUIMainObj& operator=(const EditorUIMainObj&) = delete;

    void pre_update(const EditorUpdateTick& tick);
    void update(const EditorUpdateTick& tick);
    void post_update();

    static void on_editor_event(const EditorEvent* event, void* user);

private:
    EditorContext mCtx{};
    EditorWorkspace mSceneWorkspace{};
    ViewportWindow mViewportWindow{};
    OutlinerWindow mOutlinerWindow{};
    InspectorWindow mInspectorWindow{};
    ConsoleWindow mConsoleWindow{};
    Rect mMainRect{};
    const char* mLayerName = nullptr;
    float mTopBarHeight = 0.0f;
};

EditorUIMainObj::EditorUIMainObj(const EditorUIMainInfo& mainI)
    : mCtx(mainI.ctx), mLayerName(mainI.layerName), mTopBarHeight(mainI.topBarHeight)
{
    LD_PROFILE_SCOPE;

    mCtx.add_observer(&EditorUIMainObj::on_editor_event, this);

    EditorWorkspaceInfo workspaceI{};
    workspaceI.ctx = mainI.ctx;
    workspaceI.uiContextName = EDITOR_UI_CONTEXT_NAME;
    workspaceI.uiLayerName = mainI.layerName;
    workspaceI.rootRect = Rect(0.0f, mTopBarHeight, mainI.screenSize.x, mainI.screenSize.y - mTopBarHeight);
    workspaceI.isVisible = true;
    workspaceI.isFloat = false;
    mSceneWorkspace = EditorWorkspace::create(workspaceI);

    EditorAreaID viewportArea = mSceneWorkspace.get_root_id();
    EditorAreaID outlinerArea = mSceneWorkspace.split_right(viewportArea, 0.7f);
    EditorAreaID inspectorArea = mSceneWorkspace.split_bottom(outlinerArea, 0.5f);
    EditorAreaID consoleArea = mSceneWorkspace.split_bottom(viewportArea, 0.7f);
    EditorAreaID documentArea = mSceneWorkspace.split_right(consoleArea, 0.5f);

    mViewportWindow = (ViewportWindow)mSceneWorkspace.create_window(viewportArea, EDITOR_WINDOW_VIEWPORT);
    mOutlinerWindow = (OutlinerWindow)mSceneWorkspace.create_window(outlinerArea, EDITOR_WINDOW_OUTLINER);
    mInspectorWindow = (InspectorWindow)mSceneWorkspace.create_window(inspectorArea, EDITOR_WINDOW_INSPECTOR);
    mConsoleWindow = (ConsoleWindow)mSceneWorkspace.create_window(consoleArea, EDITOR_WINDOW_CONSOLE);
    mConsoleWindow.observe_channel(get_lua_script_log_channel_name());
    mConsoleWindow.observe_channel(get_scene_log_channel_name());
    mConsoleWindow.observe_channel("EditorContext");

    DocumentWindow window = (DocumentWindow)mSceneWorkspace.create_window(documentArea, EDITOR_WINDOW_DOCUMENT);
}

EditorUIMainObj::~EditorUIMainObj()
{
    LD_PROFILE_SCOPE;

    EditorWorkspace::destroy(mSceneWorkspace);
}

void EditorUIMainObj::pre_update(const EditorUpdateTick& tick)
{
    Rect mainRect = Rect(0.0f, mTopBarHeight, tick.screenSize.x, tick.screenSize.y - mTopBarHeight);

    // with epsilon tolerance
    if (mainRect != mMainRect)
    {
        mMainRect = mainRect;
        mSceneWorkspace.set_rect(mMainRect);
    }
}

void EditorUIMainObj::update(const EditorUpdateTick& tick)
{
    LD_PROFILE_SCOPE;

    mSceneWorkspace.update(tick);
}

void EditorUIMainObj::post_update()
{
    mSceneWorkspace.post_update();
}

void EditorUIMainObj::on_editor_event(const EditorEvent* event, void* user)
{
    auto* obj = (EditorUIMainObj*)user;

    // handle requests in the main Editor region.
    if (event->category != EDITOR_EVENT_CATEGORY_REQUEST)
        return;

    switch (event->type)
    {
    case EDITOR_EVENT_TYPE_REQUEST_DOCUMENT:
        break; // TODO: async document preparation
    default:
        break;
    }
}

//
// Public API
//

EditorUIMain EditorUIMain::create(const EditorUIMainInfo& modalI)
{
    auto* obj = heap_new<EditorUIMainObj>(MEMORY_USAGE_UI, modalI);

    return EditorUIMain(obj);
}

void EditorUIMain::destroy(EditorUIMain modal)
{
    auto* obj = modal.unwrap();

    heap_delete<EditorUIMainObj>(obj);
}

void EditorUIMain::pre_update(const EditorUpdateTick& tick)
{
    mObj->pre_update(tick);
}

void EditorUIMain::update(const EditorUpdateTick& tick)
{
    mObj->update(tick);
}

void EditorUIMain::post_update()
{
    mObj->post_update();
}

void EditorUIMain::set_viewport_hover_id(SceneOverlayGizmoID gizmoID, RUID ruid)
{
    mObj->mViewportWindow.hover_id(gizmoID, ruid);
}

Camera EditorUIMain::get_viewport_camera()
{
    return mObj->mViewportWindow.get_editor_camera();
}

Camera2D EditorUIMain::get_viewport_camera_2d()
{
    return mObj->mViewportWindow.get_editor_camera_2d();
}

Vec2 EditorUIMain::get_viewport_size()
{
    return mObj->mViewportWindow.get_size();
}

Vec2 EditorUIMain::get_viewport_scene_size()
{
    return mObj->mViewportWindow.get_scene_size();
}

bool EditorUIMain::get_viewport_mouse_pos(Vec2& pickPos)
{
    return mObj->mViewportWindow.get_mouse_pos(pickPos);
}

RUID EditorUIMain::get_viewport_outline_ruid()
{
    return mObj->mCtx.get_selected_component_ruid();
}

void EditorUIMain::get_viewport_gizmo_state(SceneOverlayGizmo& gizmoType, Vec3& gizmoCenter, float& gizmoScale, RenderSystemSceneGizmoColor& gizmoColor)
{
    mObj->mViewportWindow.get_gizmo_3d_state(gizmoType, gizmoCenter, gizmoScale, gizmoColor);
}

} // namespace LD