#include <Ludens/Asset/AssetType/MeshAsset.h>
#include <Ludens/DataRegistry/DataComponent.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/UI/UIImmediate.h>
#include <LudensEditor/InspectorWindow/InspectorWindow.h>

#include "InspectComponent.h"
#include "InspectorWindowObj.h"

namespace LD {

void InspectorWindowObj::on_imgui(float delta)
{
    LD_PROFILE_SCOPE;

    root.set_color(root.get_theme().get_surface_color());
    ui_push_window(root);
    ui_top_layout_child_gap(4.0f);

    ComponentType compType;
    void* comp = ctx.get_component(subjectID, &compType);

    if (subjectID != (CUID)0)
        eui_inspect_component(*this, compType, comp);

    ui_pop_window();
}

void InspectorWindowObj::request_new_asset(AssetType type, AUID currentID)
{
    isRequestingNewAsset.set(true);
    requestAssetType = type;
    oldAssetID = currentID;
}

void InspectorWindowObj::on_editor_event(const EditorEvent* event, void* user)
{
    auto& self = *(InspectorWindowObj*)user;

    if (event->type != EDITOR_EVENT_TYPE_NOTIFY_COMPONENT_SELECTION)
        return;

    const auto* selectionEvent = static_cast<const EditorNotifyComponentSelectionEvent*>(event);
    self.subjectID = selectionEvent->component;
}

//
// Public API
//

EditorWindow InspectorWindow::create(const EditorWindowInfo& windowI)
{
    InspectorWindowObj* obj = heap_new<InspectorWindowObj>(MEMORY_USAGE_UI);
    obj->ctx = windowI.ctx;
    obj->ctx.add_observer(&InspectorWindowObj::on_editor_event, obj);
    obj->space = windowI.space;
    obj->root = obj->space.create_window(obj->space.get_root_id(), obj->ctx.make_vbox_layout(), {}, nullptr);

    return EditorWindow((EditorWindowObj*)obj);
}

void InspectorWindow::destroy(EditorWindow window)
{
    LD_ASSERT(window && window.get_type() == EDITOR_WINDOW_INSPECTOR);
    auto* obj = static_cast<InspectorWindowObj*>(window.unwrap());

    heap_delete<InspectorWindowObj>(obj);
}

bool InspectorWindow::has_component_asset_request(CUID& compID, AUID& currentAssetID, AssetType& assetType)
{
    if (!mObj->isRequestingNewAsset.read())
        return false;

    compID = mObj->subjectID;
    currentAssetID = mObj->oldAssetID;
    assetType = mObj->requestAssetType;

    return true;
}

} // namespace LD