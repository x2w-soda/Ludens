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

struct EditorWorkspaceLayout
{
    std::string uiLayerName;
    EditorWorkspace workspace;

    EditorWorkspaceLayout(EditorContext ctx, Rect rootRect, const char* uiLayerName);
    EditorWorkspaceLayout(const EditorWorkspaceLayout&) = delete;
    ~EditorWorkspaceLayout();

    EditorWorkspaceLayout& operator=(const EditorWorkspaceLayout&) = delete;
};

EditorWorkspaceLayout::EditorWorkspaceLayout(EditorContext ctx, Rect rootRect, const char* uiLayerName)
{
    EditorWorkspaceInfo workspaceI{};
    workspaceI.ctx = ctx;
    workspaceI.uiContextName = EDITOR_UI_CONTEXT_NAME;
    workspaceI.uiLayerName = uiLayerName;
    workspaceI.rootRect = rootRect;
    workspaceI.isVisible = false;
    workspaceI.isFloat = false;
    workspace = EditorWorkspace::create(workspaceI);

    this->uiLayerName = uiLayerName;
}

EditorWorkspaceLayout::~EditorWorkspaceLayout()
{
    EditorWorkspace::destroy(workspace);
}

class EditorUIMainObj
{
    friend class EditorUIMain;

public:
    EditorUIMainObj(const EditorUIMainInfo& mainI);
    EditorUIMainObj(const EditorUIMainObj&) = delete;
    ~EditorUIMainObj();

    EditorUIMainObj& operator=(const EditorUIMainObj&) = delete;

    EditorWorkspace get_active_workspace();
    void pre_update(const EditorUpdateTick& tick);
    void update(const EditorUpdateTick& tick);
    void post_update();

    static void on_editor_event(const EditorEvent* event, void* user);

private:
    EditorContext mCtx{};
    EditorUIMainLayout mActiveLayout = {};
    Vector<EditorWorkspaceLayout*> mLayouts;
    ViewportWindow mViewportWindow = {};
    OutlinerWindow mOutlinerWindow = {};
    InspectorWindow mInspectorWindow = {};
    ConsoleWindow mConsoleWindow = {};
    Rect mMainRect = {};
    float mTopBarHeight = 0.0f;
};

EditorUIMainObj::EditorUIMainObj(const EditorUIMainInfo& mainI)
    : mCtx(mainI.ctx), mTopBarHeight(mainI.topBarHeight)
{
    LD_PROFILE_SCOPE;

    mCtx.add_observer(&EditorUIMainObj::on_editor_event, this);

    Rect rootRect(0.0f, mTopBarHeight, mainI.screenSize.x, mainI.screenSize.y - mTopBarHeight);

    mLayouts.resize(2);
    mLayouts[0] = heap_new<EditorWorkspaceLayout>(MEMORY_USAGE_UI, mainI.ctx, rootRect, EDITOR_UI_LAYER_MAIN_SCENE_NAME);
    mLayouts[0]->workspace.set_visible(true);

    EditorWorkspace workspace = mLayouts[0]->workspace;
    EditorAreaID viewportArea = workspace.get_root_id();
    EditorAreaID outlinerArea = workspace.split_right(viewportArea, 0.7f);
    EditorAreaID inspectorArea = workspace.split_bottom(outlinerArea, 0.5f);
    EditorAreaID consoleArea = workspace.split_bottom(viewportArea, 0.7f);

    mViewportWindow = (ViewportWindow)workspace.create_window(viewportArea, EDITOR_WINDOW_VIEWPORT);
    mOutlinerWindow = (OutlinerWindow)workspace.create_window(outlinerArea, EDITOR_WINDOW_OUTLINER);
    mInspectorWindow = (InspectorWindow)workspace.create_window(inspectorArea, EDITOR_WINDOW_INSPECTOR);
    mConsoleWindow = (ConsoleWindow)workspace.create_window(consoleArea, EDITOR_WINDOW_CONSOLE);
    mConsoleWindow.observe_channel(get_lua_script_log_channel_name());
    mConsoleWindow.observe_channel(get_scene_log_channel_name());
    mConsoleWindow.observe_channel("EditorContext");

    mLayouts[1] = heap_new<EditorWorkspaceLayout>(MEMORY_USAGE_UI, mainI.ctx, rootRect, EDITOR_UI_LAYER_MAIN_DOCS_NAME);

    workspace = mLayouts[1]->workspace;
    EditorAreaID docLArea = workspace.get_root_id();
    EditorAreaID docRArea = workspace.split_right(docLArea, 0.5f);
    DocumentWindow documentWindow = {};
    documentWindow = (DocumentWindow)workspace.create_window(docLArea, EDITOR_WINDOW_DOCUMENT);
    documentWindow = (DocumentWindow)workspace.create_window(docRArea, EDITOR_WINDOW_DOCUMENT);

    mActiveLayout = EDITOR_UI_MAIN_LAYOUT_SCENE;
}

EditorUIMainObj::~EditorUIMainObj()
{
    LD_PROFILE_SCOPE;

    for (EditorWorkspaceLayout* layout : mLayouts)
        heap_delete<EditorWorkspaceLayout>(layout);
}

EditorWorkspace EditorUIMainObj::get_active_workspace()
{
    LD_ASSERT((size_t)mActiveLayout < mLayouts.size());

    return mLayouts[(int)mActiveLayout]->workspace;
}

void EditorUIMainObj::pre_update(const EditorUpdateTick& tick)
{
    mMainRect = Rect(0.0f, mTopBarHeight, tick.screenSize.x, tick.screenSize.y - mTopBarHeight);

    EditorWorkspace workspace = get_active_workspace();
    workspace.set_rect(mMainRect);
    workspace.pre_update(tick);
}

void EditorUIMainObj::update(const EditorUpdateTick& tick)
{
    LD_PROFILE_SCOPE;

    EditorWorkspace workspace = get_active_workspace();
    workspace.update(tick);
}

void EditorUIMainObj::post_update()
{
    EditorWorkspace workspace = get_active_workspace();
    workspace.post_update();
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

void EditorUIMain::set_layout(EditorUIMainLayout layoutType)
{
    EditorWorkspaceLayout* layout = mObj->mLayouts[(int)mObj->mActiveLayout];
    layout->workspace.set_visible(false);

    // make sure we explicitly set the old layout as invisible right now.
    ui_imgui_set_layer_visible(EDITOR_UI_CONTEXT_NAME, layout->uiLayerName.c_str(), false);

    mObj->mActiveLayout = layoutType;
    layout = mObj->mLayouts[(int)mObj->mActiveLayout];
    layout->workspace.set_visible(true);
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