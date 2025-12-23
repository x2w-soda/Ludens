#include <Ludens/Asset/AssetType/MeshAsset.h>
#include <Ludens/DataRegistry/DataComponent.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/System/Memory.h>
#include <Ludens/UI/UIImmediate.h>
#include <LudensEditor/InspectorWindow/InspectorWindow.h>

#include "InspectComponent.h"
#include "InspectorWindowObj.h"

namespace LD {

void EInspectorWindowObj::inspect_component(CUID compID)
{
    LD_PROFILE_SCOPE;

    subjectID = compID;
    isSelectingNewAsset = false;
}

void EInspectorWindowObj::on_imgui()
{
    ui_push_window("EInspectorWindow", root);

    ComponentType compType;
    void* comp = editorCtx.get_component(subjectID, &compType);

    if (subjectID == (CUID)0)
    {
        ui_pop_window();
        return;
    }

    eui_inspect_component(*this, compType, comp);

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
    void* comp = mObj->editorCtx.get_component(mObj->subjectID, &compType);

    if (!comp)
        return;

    Scene scene = mObj->editorCtx.get_scene();

    switch (compType)
    {
    case COMPONENT_TYPE_AUDIO_SOURCE: {
        Scene::IAudioSource source((AudioSourceComponent*)comp);
        source.set_clip_asset(assetID);
        break;
    }
    case COMPONENT_TYPE_MESH: {
        Scene::IMesh mesh(mObj->subjectID);
        mesh.set_mesh_asset(assetID);
        break;
    }
    case COMPONENT_TYPE_SPRITE_2D: {
        Scene::ISprite2D sprite((Sprite2DComponent*)comp);
        sprite.set_texture_2d_asset(assetID);
        break;
    }
    default:
        break;
    }

    mObj->isSelectingNewAsset = false;
}

} // namespace LD