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

void InspectorWindowObj::on_imgui(float delta)
{
    LD_PROFILE_SCOPE;

    EditorTheme theme = mCtx.get_theme();
    UILayoutInfo layoutI = theme.make_vbox_layout_fixed(mRootRect.get_size());
    layoutI.childGap = 4.0f;

    ui_workspace_begin();
    ui_push_window(ui_workspace_name());

    ui_window_set_color(theme.get_ui_theme().get_surface_color());
    ui_top_layout(layoutI);

    ComponentView comp = mCtx.get_component(mCtx.get_selected_component());
    if (comp)
        eui_inspect_component(&storage, comp);

    ui_pop_window();
    ui_workspace_end();
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
    LD_ASSERT(window && window.get_type() == EDITOR_WINDOW_INSPECTOR);
    auto* obj = static_cast<InspectorWindowObj*>(window.unwrap());

    heap_delete<InspectorWindowObj>(obj);
}

} // namespace LD