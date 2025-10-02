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

    CUID subjectID = 0; // subject component being inspected
    UITransformEditWidget transformEditW;
    UIAssetSlotWidget slotW;
    UIAssetSlotWidget requestingNewAsset = {};
    ECBSelectAssetFn selectAssetFn;
    void* user;

    void inspect_component(CUID compID);

    static void on_draw(UIWidget widget, ScreenRenderComponent renderer);
    static void on_editor_context_event(const EditorContextEvent* event, void* user);
    static void on_request_new_asset(AssetType type, AUID currentID, void* user);
};

void EInspectorWindowObj::inspect_component(CUID compID)
{
    LD_PROFILE_SCOPE;

    subjectID = compID;
    requestingNewAsset = {};

    AssetManager AM = editorCtx.get_asset_manager();

    if (subjectID == 0)
    {
        transformEditW.set(nullptr);
        transformEditW.hide();
        slotW.hide();
        return;
    }

    transformEditW.show();
    slotW.show();

    ComponentType compType;
    void* comp = editorCtx.get_component(subjectID, compType);

    // TODO: map component type to its required widgets for inspection.
    switch (compType)
    {
    case COMPONENT_TYPE_MESH: {
        MeshComponent* meshC = (MeshComponent*)comp;
        transformEditW.set(&meshC->transform);
        MeshAsset asset = AM.get_mesh_asset(meshC->auid);
        LD_ASSERT(asset);
        slotW.set_asset(meshC->auid, asset.get_name());
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

void EInspectorWindowObj::on_request_new_asset(AssetType type, AUID currentID, void* user)
{
    auto& self = *(EInspectorWindowObj*)user;

    // requesting a new Asset is beyond the scope of the inspector window
    self.requestingNewAsset = self.slotW;

    if (self.selectAssetFn)
        self.selectAssetFn(type, currentID, self.user);
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
    obj->selectAssetFn = windowI.selectAssetFn;
    obj->user = windowI.user;

    EditorTheme theme = obj->editorCtx.get_settings().get_theme();
    UITransformEditWidgetInfo transformEditWI{};
    transformEditWI.parent = obj->root;
    transformEditWI.theme = theme;
    obj->transformEditW = UITransformEditWidget::create(transformEditWI);
    obj->transformEditW.hide();

    UIAssetSlotWidgetInfo slotWI{};
    slotWI.parent = obj->root;
    slotWI.theme = theme;
    slotWI.assetID = 0;
    slotWI.assetName = nullptr;
    slotWI.type = ASSET_TYPE_MESH;
    slotWI.requestAssetFn = &EInspectorWindowObj::on_request_new_asset;
    slotWI.user = obj;
    obj->slotW = UIAssetSlotWidget::create(slotWI);
    obj->slotW.hide();

    float pad = theme.get_padding();
    UIWindow inspectorWindow = wm.get_area_window(windowI.areaID);
    UIPadding padding{};
    padding.left = pad;
    padding.right = pad;
    inspectorWindow.set_layout_child_padding(padding);

    obj->editorCtx.add_observer(&EInspectorWindowObj::on_editor_context_event, obj);

    return {obj};
}

void EInspectorWindow::destroy(EInspectorWindow window)
{
    EInspectorWindowObj* obj = window;

    heap_delete<EInspectorWindowObj>(obj);
}

void EInspectorWindow::select_asset(AUID assetID, const char* assetName)
{
    if (!mObj->requestingNewAsset)
        return;

    UIAssetSlotWidget slotW = mObj->requestingNewAsset;
    slotW.set_asset(assetID, assetName);

    ComponentType compType;
    MeshComponent* comp = (MeshComponent*)mObj->editorCtx.get_component(mObj->subjectID, compType);
    LD_ASSERT(comp && compType == COMPONENT_TYPE_MESH);
    mObj->editorCtx.set_mesh_component_asset(mObj->subjectID, assetID);

    mObj->requestingNewAsset = {};
}

} // namespace LD