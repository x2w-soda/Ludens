#include <Ludens/Memory/Memory.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Scene/Scene.h>
#include <LudensEditor/ConsoleWindow/ConsoleWindow.h>
#include <LudensEditor/InspectorWindow/InspectorWindow.h>
#include <LudensEditor/OutlinerWindow/OutlinerWindow.h>
#include <LudensEditor/SelectionWindow/SelectionWindow.h>
#include <LudensEditor/VersionWindow/VersionWindow.h>
#include <LudensEditor/ViewportWindow/ViewportWindow.h>

#include <LudensEditor/EditorUI/EditorUIMain.h>
#include <LudensEditor/EditorUI/EditorWorkspace.h>

namespace LD {

class EditorUIMainObj
{
    friend class EditorUIMain;

public:
    EditorUIMainObj(const EditorUIMainInfo& mainI);
    EditorUIMainObj(const EditorUIMainObj&) = delete;
    ~EditorUIMainObj();

    EditorUIMainObj& operator=(const EditorUIMainObj&) = delete;

    void on_imgui(float delta);
    void update(float delta);
    void resize(const Vec2& screenSize);

private:
    EditorContext mCtx{};
    EditorWorkspace mSceneWorkspace{};
    ViewportWindow mViewportWindow{};
    OutlinerWindow mOutlinerWindow{};
    InspectorWindow mInspectorWindow{};
    ConsoleWindow mConsoleWindow{};
    float mTopBarHeight = 0.0f;
};

EditorUIMainObj::EditorUIMainObj(const EditorUIMainInfo& mainI)
    : mCtx(mainI.ctx), mTopBarHeight(mainI.topBarHeight)
{
    LD_PROFILE_SCOPE;

    EditorWorkspaceInfo workspaceI{};
    workspaceI.ctx = mainI.ctx;
    workspaceI.layer = mainI.groundLayer;
    workspaceI.rootRect = Rect(0.0f, mTopBarHeight, mainI.screenSize.x, mainI.screenSize.y);
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
}

EditorUIMainObj::~EditorUIMainObj()
{
    LD_PROFILE_SCOPE;

    EditorWorkspace::destroy(mSceneWorkspace);
}

void EditorUIMainObj::on_imgui(float delta)
{
    LD_PROFILE_SCOPE;

    mSceneWorkspace.on_imgui(delta);
}

void EditorUIMainObj::update(float delta)
{
    LD_PROFILE_SCOPE;

    CUID subjectCompID;
    AUID oldAssetID = 0;
    AssetType type;

    if (mInspectorWindow.has_component_asset_request(subjectCompID, oldAssetID, type))
    {
        // NOTE: will have to refactor, this assumes single asset slot for all component types
        EditorRequestComponentAssetEvent event(subjectCompID, oldAssetID, type);

        mCtx.request_event(&event);
    }
}

void EditorUIMainObj::resize(const Vec2& screenSize)
{
    Rect groundRect = Rect(0.0f, mTopBarHeight, screenSize.x, screenSize.y - mTopBarHeight);

    mSceneWorkspace.set_rect(groundRect);
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

void EditorUIMain::on_imgui(float delta)
{
    mObj->on_imgui(delta);
}

void EditorUIMain::update(float delta)
{
    mObj->update(delta);
}

void EditorUIMain::resize(const Vec2& screenSize)
{
    LD_ASSERT(screenSize.x > 0.0f && screenSize.y > 0.0f);

    mObj->resize(screenSize);
}

void EditorUIMain::set_viewport_hover_id(SceneOverlayGizmoID gizmoID, RUID ruid)
{
    mObj->mViewportWindow.hover_id(gizmoID, ruid);
}

Camera EditorUIMain::get_viewport_camera()
{
    return mObj->mViewportWindow.get_editor_camera();
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

void EditorUIMain::get_viewport_gizmo_state(SceneOverlayGizmo& gizmoType, Vec3& gizmoCenter, float& gizmoScale, RenderServerSceneGizmoColor& gizmoColor)
{
    mObj->mViewportWindow.get_gizmo_state(gizmoType, gizmoCenter, gizmoScale, gizmoColor);
}

} // namespace LD