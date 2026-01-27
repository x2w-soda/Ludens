#include <Ludens/Profiler/Profiler.h>
#include <Ludens/UI/UIContext.h>
#include <Ludens/UI/UIImmediate.h>
#include <LudensEditor/EditorUI/EditorDialog.h>
#include <LudensEditor/EditorUI/EditorWorkspace.h>
#include <LudensEditor/SelectionWindow/SelectionWindow.h>

namespace LD {

/// @brief Editor dialog implementation.
struct EditorDialogObj
{
    EditorContext ctx;
    EditorWorkspace workspace;
    EditorWindow window;
    UIContext uiCtx;
    UILayer uiLayer;
    WindowID windowID = 0;

    static void on_observer_event(const WindowEvent* event, void* user);
    static void on_event(const WindowEvent* event, void* user);
};

void EditorDialogObj::on_observer_event(const WindowEvent* event, void* user)
{
    auto* obj = (EditorDialogObj*)user;

    if (event->window != obj->windowID)
        return;

    if (event->type == EVENT_TYPE_WINDOW_DESTROY)
    {
        EditorWorkspace::destroy(obj->workspace);
        obj->workspace = {};
        obj->window = {};
        obj->windowID = 0;
    }
}

void EditorDialogObj::on_event(const WindowEvent* event, void* user)
{
    auto* obj = (EditorDialogObj*)user;

    obj->uiCtx.on_window_event(event);
}

//
// Public API
//

EditorDialog EditorDialog::create(const EditorDialogInfo& dialogI)
{
    LD_PROFILE_SCOPE;

    auto* obj = heap_new<EditorDialogObj>(MEMORY_USAGE_UI);

    obj->ctx = dialogI.ctx;

    UIContextInfo uiCtxI{};
    uiCtxI.fontAtlas = dialogI.fontAtlas;
    uiCtxI.fontAtlasImage = dialogI.fontAtlasImage;
    uiCtxI.theme = obj->ctx.get_theme().get_ui_theme();
    obj->uiCtx = UIContext::create(uiCtxI);
    obj->uiLayer = obj->uiCtx.create_layer("dialog");

    EditorWorkspaceInfo wsI{};
    wsI.ctx = obj->ctx;
    wsI.isFloat = false;
    wsI.isVisible = true;
    wsI.layer = obj->uiLayer;
    wsI.rootRect = Rect(0.0f, 0.0f, dialogI.extent.x, dialogI.extent.y);
    obj->workspace = EditorWorkspace::create(wsI);
    obj->window = obj->workspace.create_window(obj->workspace.get_root_id(), dialogI.type);

    WindowRegistry reg = WindowRegistry::get();
    WindowInfo windowI{};
    windowI.width = dialogI.extent.x;
    windowI.height = dialogI.extent.y;
    windowI.onEvent = &EditorDialogObj::on_event;
    windowI.name = "DialogWindow";
    windowI.user = obj;
    obj->windowID = reg.create_window(windowI, reg.get_root_id());

    reg.add_observer(&EditorDialogObj::on_observer_event, obj);

    return EditorDialog(obj);
}

void EditorDialog::destroy(EditorDialog dialog)
{
    LD_PROFILE_SCOPE;

    auto* obj = dialog.unwrap();

    WindowRegistry reg = WindowRegistry::get();
    reg.close_window(obj->windowID);
    obj->windowID = 0;

    if (obj->workspace)
        EditorWorkspace::destroy(obj->workspace);

    UIContext::destroy(obj->uiCtx);

    heap_delete<EditorDialogObj>(obj);
}

void EditorDialog::update(float delta)
{
    LD_PROFILE_SCOPE;

    if (!mObj->workspace)
        return;

    ui_frame_begin(mObj->uiCtx);
    mObj->workspace.on_imgui(delta);
    ui_frame_end();

    mObj->uiCtx.update(delta);
}

void EditorDialog::render(ScreenRenderComponent renderer)
{
    mObj->uiLayer.render(renderer);
}

EditorWindow EditorDialog::get_editor_window(EditorWindowType typeCheck)
{
    if (!mObj->workspace || !mObj->window || mObj->window.get_type() != typeCheck)
        return {};

    return mObj->window;
}

WindowID EditorDialog::get_id()
{
    return mObj->windowID;
}

} // namespace LD