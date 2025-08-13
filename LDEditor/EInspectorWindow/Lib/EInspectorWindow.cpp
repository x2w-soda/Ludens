#include <Ludens/DataRegistry/DataComponent.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/System/Memory.h>
#include <LudensEditor/EInspectorWindow/EInspectorWindow.h>
#include <LudensEditor/EditorContext/EditorWidget.h>
#include <LudensEditor/EditorContext/EditorWindowObj.h>

namespace LD {

/// @brief Editor inspector window implementation.
struct EInspectorWindowObj : EditorWindowObj
{
    virtual ~EInspectorWindowObj() = default;

    UITransformEditWidget transformEdit;

    void inspect_component(DUID compID);

    static void on_draw(UIWidget widget, ScreenRenderComponent renderer);
    static void on_editor_context_event(const EditorContextEvent* event, void* user);
};

void EInspectorWindowObj::inspect_component(DUID compID)
{
    LD_PROFILE_SCOPE;

    if (compID == 0)
    {
        transformEdit.set(nullptr);
        return;
    }

    ComponentType compType;
    void* comp = editorCtx.get_component(compID, compType);

    // TODO: map component type to its required widgets for inspection.
    switch (compType)
    {
    case COMPONENT_TYPE_MESH:
        transformEdit.set(&((MeshComponent*)comp)->transform);
    }
}

void EInspectorWindowObj::on_draw(UIWidget widget, ScreenRenderComponent renderer)
{
    UIWindow window = (UIWindow)widget;
    EInspectorWindowObj& self = *(EInspectorWindowObj*)window.get_user();
    EditorTheme theme = self.editorCtx.get_settings().get_theme();
    Rect windowRect = window.get_rect();

    Color bgColor;
    theme.get_background_color(bgColor);

    renderer.push_scissor(windowRect);
    renderer.draw_rect(windowRect, bgColor);

    self.transformEdit.on_draw(renderer);

    renderer.pop_scissor();
}

void EInspectorWindowObj::on_editor_context_event(const EditorContextEvent* event, void* user)
{
    auto& self = *(EInspectorWindowObj*)user;

    if (event->type != EDITOR_CONTEXT_EVENT_COMPONENT_SELECTION)
        return;

    const auto* selectionEvent = static_cast<const EditorContextComponentSelectionEvent*>(event);
    self.inspect_component(selectionEvent->component);
}

EInspectorWindow EInspectorWindow::create(const EInspectorWindowInfo& windowI)
{
    UIWindowManager wm = windowI.wm;

    wm.set_window_title(windowI.areaID, "Inspector");

    EInspectorWindowObj* obj = heap_new<EInspectorWindowObj>(MEMORY_USAGE_UI);
    obj->editorCtx = windowI.ctx;
    obj->root = wm.get_area_window(windowI.areaID);
    obj->root.set_user(obj);
    obj->root.set_on_draw(&EInspectorWindowObj::on_draw);

    EditorTheme theme = obj->editorCtx.get_settings().get_theme();
    UITransformEditWidgetInfo transformEditWI{};
    transformEditWI.parent = obj->root;
    transformEditWI.theme = theme;
    obj->transformEdit = UITransformEditWidget::create(transformEditWI);

    obj->editorCtx.add_observer(&EInspectorWindowObj::on_editor_context_event, obj);

    return {obj};
}

void EInspectorWindow::destroy(EInspectorWindow window)
{
    EInspectorWindowObj* obj = window;

    heap_delete<EInspectorWindowObj>(obj);
}

} // namespace LD