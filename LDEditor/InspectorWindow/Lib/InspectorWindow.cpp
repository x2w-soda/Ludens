#include <Ludens/Asset/AssetType/MeshAsset.h>
#include <Ludens/DataRegistry/DataRegistry.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/UI/UIImmediate.h>
#include <LudensEditor/InspectorWindow/InspectorWindow.h>

#include "InspectComponent.h"
#include "InspectorWindowObj.h"

namespace LD {

void InspectorWindowObj::update(float delta)
{
    LD_PROFILE_SCOPE;

    begin_update_window();

    UILayoutInfo layoutI = theme.make_vbox_layout_fixed(rootRect.get_size());
    layoutI.childGap = 4.0f;

    ui_window_set_color(theme.get_ui_theme().get_surface_color());
    ui_top_layout(layoutI);

    ComponentView comp = ctx.get_component(ctx.get_selected_component());
    if (comp)
        eui_inspect_component(&storage, comp);

    end_update_window();
}

//
// Public API
//

EditorWindow InspectorWindow::create(const EditorWindowInfo& windowI)
{
    InspectorWindowObj* obj = heap_new<InspectorWindowObj>(MEMORY_USAGE_UI, windowI);

    return EditorWindow((EditorWindowObj*)obj);
}

void InspectorWindow::destroy(EditorWindow window)
{
    auto* obj = static_cast<InspectorWindowObj*>(window.unwrap());

    heap_delete<InspectorWindowObj>(obj);
}

void InspectorWindow::update(EditorWindowObj* base, const EditorUpdateTick& tick)
{
    auto* obj = static_cast<InspectorWindowObj*>(base);

    obj->update(tick.delta);
}

} // namespace LD
