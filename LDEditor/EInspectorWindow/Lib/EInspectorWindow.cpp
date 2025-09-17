#include <Ludens/Asset/MeshAsset.h>
#include <Ludens/DataRegistry/DataComponent.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/System/Memory.h>
#include <LudensEditor/EInspectorWindow/EInspectorWindow.h>
#include <LudensEditor/EditorContext/EditorWindowObj.h>
#include <LudensEditor/EditorWidget/UIAssetSlotWidget.h>
#include <LudensEditor/EditorWidget/UITransformEditWidget.h>

namespace LD {

/// @brief Editor inspector window implementation.
struct EInspectorWindowObj : EditorWindowObj
{
    virtual ~EInspectorWindowObj() = default;

    UITransformEditWidget transformEditW;
    UIAssetSlotWidget slotW;

    void inspect_component(CUID compID);

    static void on_draw(UIWidget widget, ScreenRenderComponent renderer);
    static void on_editor_context_event(const EditorContextEvent* event, void* user);
};

void EInspectorWindowObj::inspect_component(CUID compID)
{
    LD_PROFILE_SCOPE;

    AssetManager AM = editorCtx.get_asset_manager();

    if (compID == 0)
    {
        transformEditW.set(nullptr);
        transformEditW.hide();
        slotW.hide();
        return;
    }

    transformEditW.show();
    slotW.show();

    ComponentType compType;
    void* comp = editorCtx.get_component(compID, compType);

    // TODO: map component type to its required widgets for inspection.
    switch (compType)
    {
    case COMPONENT_TYPE_MESH: {
        MeshComponent* meshC = (MeshComponent*)comp;
        transformEditW.set(&meshC->transform);
        slotW.set(&meshC->auid);
        const char* assetName = nullptr;
        MeshAsset asset = AM.get_mesh_asset(meshC->auid);
        if (asset)
            assetName = asset.get_name();
        slotW.set_asset_name(assetName);
        break;
    }
    default:
        break;
    }
}

void EInspectorWindowObj::on_draw(UIWidget widget, ScreenRenderComponent renderer)
{
    UITheme theme = widget.get_theme();
    Rect windowRect = widget.get_rect();
    Color color = theme.get_surface_color();
    renderer.draw_rect(windowRect, color);
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
    obj->root.set_on_draw(EInspectorWindowObj::on_draw);

    EditorTheme theme = obj->editorCtx.get_settings().get_theme();
    UITransformEditWidgetInfo transformEditWI{};
    transformEditWI.parent = obj->root;
    transformEditWI.theme = theme;
    obj->transformEditW = UITransformEditWidget::create(transformEditWI);
    obj->transformEditW.hide();

    // TODO:
    UIAssetSlotWidgetInfo slotWI{};
    slotWI.parent = obj->root;
    slotWI.theme = theme;
    slotWI.asset = nullptr;
    slotWI.type = ASSET_TYPE_MESH;
    obj->slotW = UIAssetSlotWidget::create(slotWI);
    obj->slotW.hide();

    obj->editorCtx.add_observer(&EInspectorWindowObj::on_editor_context_event, obj);

    return {obj};
}

void EInspectorWindow::destroy(EInspectorWindow window)
{
    EInspectorWindowObj* obj = window;

    heap_delete<EInspectorWindowObj>(obj);
}

} // namespace LD