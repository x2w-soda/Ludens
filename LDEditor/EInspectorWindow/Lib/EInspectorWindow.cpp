#include <Ludens/Asset/AssetType/MeshAsset.h>
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
    ECBSelectAssetFn selectAssetFn;
    void* user;
    bool isSelectingNewAsset = false;

    void inspect_component(CUID compID);

    virtual void on_imgui() override;

    static void on_draw(UIWidget widget, ScreenRenderComponent renderer);
    static void on_editor_context_event(const EditorContextEvent* event, void* user);
};

void EInspectorWindowObj::inspect_component(CUID compID)
{
    LD_PROFILE_SCOPE;

    subjectID = compID;
    isSelectingNewAsset = false;
}

void EInspectorWindowObj::on_imgui()
{
    ui_push_window("EInspectorWindow", root);

    EditorTheme editorTheme = editorCtx.get_settings().get_theme();
    AssetManager AM = editorCtx.get_asset_manager();
    ComponentType compType;
    void* comp = editorCtx.get_component(subjectID, compType);

    if (subjectID == (CUID)0 || compType != COMPONENT_TYPE_MESH)
    {
        ui_pop_window();
        return;
    }

    MeshComponent* meshC = (MeshComponent*)comp;
    eui_transform_edit(editorTheme, &meshC->transform);

    MeshAsset asset = AM.get_mesh_asset(meshC->auid);
    LD_ASSERT(asset);
    if (eui_asset_slot(editorTheme, ASSET_TYPE_MESH, meshC->auid, asset.get_name()) && selectAssetFn)
    {
        isSelectingNewAsset = true;
        selectAssetFn(ASSET_TYPE_MESH, meshC->auid, user);
    }

    ui_pop_window();
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
    UIWindow window = wm.get_area_window(windowI.areaID);

    EInspectorWindowObj* obj = heap_new<EInspectorWindowObj>(MEMORY_USAGE_UI);
    obj->editorCtx = windowI.ctx;
    obj->root = window;
    obj->root.set_user(obj);
    obj->root.set_on_draw(EInspectorWindowObj::on_draw);
    obj->selectAssetFn = windowI.selectAssetFn;
    obj->user = windowI.user;

    EditorTheme theme = obj->editorCtx.get_settings().get_theme();
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

void EInspectorWindow::select_asset(AUID assetID)
{
    if (!mObj->isSelectingNewAsset || !mObj->subjectID)
        return;

    ComponentType compType;
    MeshComponent* comp = (MeshComponent*)mObj->editorCtx.get_component(mObj->subjectID, compType);
    LD_ASSERT(comp && compType == COMPONENT_TYPE_MESH);
    mObj->editorCtx.set_mesh_component_asset(mObj->subjectID, assetID);

    mObj->isSelectingNewAsset = false;
}

} // namespace LD