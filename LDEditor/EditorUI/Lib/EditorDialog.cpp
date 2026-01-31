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

    void destroy();

    static void on_observer_event(const WindowEvent* event, void* user);
    static void on_event(const WindowEvent* event, void* user);
};

void EditorDialogObj::destroy()
{
    LD_PROFILE_SCOPE;

    if (!uiCtx)
        return;

    LD_ASSERT(windowID && workspace);

    WindowRegistry reg = WindowRegistry::get();
    reg.remove_observer(&on_observer_event, this);
    reg.close_window(windowID);
    windowID = 0;

    EditorWorkspace::destroy(workspace);
    workspace = {};
    window = {};

    UIContext::destroy(uiCtx);
    uiCtx = {};
    uiLayer = {};
}

void EditorDialogObj::on_observer_event(const WindowEvent* event, void* user)
{
    auto* obj = (EditorDialogObj*)user;

    if (event->window != obj->windowID)
        return;

    if (event->type == EVENT_TYPE_WINDOW_DESTROY)
    {
        obj->destroy();
    }
}

void EditorDialogObj::on_event(const WindowEvent* event, void* user)
{
    auto* obj = (EditorDialogObj*)user;

    switch (event->type)
    {
    case EVENT_TYPE_WINDOW_RESIZE:
    {
        const auto* e = (const WindowResizeEvent*)event;
        obj->workspace.set_rect(Rect(0.0f, 0.0f, (float)e->width, (float)e->height));
        break;
    }
    default:
        obj->uiCtx.on_window_event(event);
        break;
    }
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
    windowI.hintBorderColor = 0;
    windowI.hintTitleBarColor = 0x000000FF;
    windowI.hintTitleBarTextColor = 0xDFDFDFFF;
    obj->windowID = reg.create_window(windowI, reg.get_root_id());

    reg.add_observer(&EditorDialogObj::on_observer_event, obj);

    return EditorDialog(obj);
}

void EditorDialog::destroy(EditorDialog dialog)
{
    LD_PROFILE_SCOPE;

    auto* obj = dialog.unwrap();
    obj->destroy();

    heap_delete<EditorDialogObj>(obj);
}

void EditorDialog::update(float delta)
{
    LD_PROFILE_SCOPE;

    if (!mObj->workspace)
        return;

    if (mObj->workspace.should_close())
    {
        mObj->destroy();
        return;
    }

    ui_frame_begin(mObj->uiCtx);
    mObj->workspace.on_imgui(delta);
    ui_frame_end();

    mObj->uiCtx.update(delta);
}

void EditorDialog::render(ScreenRenderComponent renderer)
{
    mObj->uiLayer.render(renderer);
}

bool EditorDialog::should_close()
{
    return !mObj->uiCtx;
}

EditorWindow EditorDialog::get_editor_window(EditorWindowType typeCheck)
{
    if (!mObj->window || mObj->window.get_type() != typeCheck)
        return {};

    return mObj->window;
}

WindowID EditorDialog::get_id()
{
    return mObj->windowID;
}

} // namespace LD