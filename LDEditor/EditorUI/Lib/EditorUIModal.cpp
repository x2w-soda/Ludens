#include <LudensEditor/EditorUI/EditorUIModal.h>
#include <LudensEditor/EditorUI/EditorWorkspace.h>

#include <Ludens/WindowRegistry/Input.h>

namespace LD {

struct EditorUIModalObj
{
    EditorContext ctx;
    EditorWorkspace backdrop;
    EditorWorkspace modal;
    const char* layerName = nullptr;
    bool isVisible = false;
};

EditorUIModal EditorUIModal::create(const EditorUIModalInfo& modalI)
{
    auto* obj = heap_new<EditorUIModalObj>(MEMORY_USAGE_UI);
    obj->ctx = modalI.ctx;
    obj->layerName = modalI.layerName;
    obj->isVisible = false;

    EditorWorkspaceInfo spaceI{};
    spaceI.ctx = obj->ctx;
    spaceI.uiLayerName = "EDITOR_UI_LAYER_MODAL_BACKDROP";
    spaceI.isFloat = false;
    spaceI.isVisible = obj->isVisible;
    spaceI.rootRect = Rect(0.0f, 0.0f, modalI.screenSize.x, modalI.screenSize.y);
    spaceI.rootColor = 0x10101070;
    obj->backdrop = EditorWorkspace::create(spaceI);

    const float modalW = 500.0f;
    const float modalH = 500.0f;
    spaceI.uiLayerName = modalI.layerName;
    spaceI.isFloat = true;
    spaceI.isVisible = obj->isVisible;
    spaceI.rootRect = Rect((modalI.screenSize.x - modalW) / 2.0f, (modalI.screenSize.y - modalH) / 2.0f, modalW, modalH);
    spaceI.rootColor = 0;
    obj->modal = EditorWorkspace::create(spaceI);
    obj->modal.create_window(obj->modal.get_root_id(), EDITOR_WINDOW_PROJECT);

    return EditorUIModal(obj);
}

void EditorUIModal::destroy(EditorUIModal modal)
{
    auto* obj = modal.unwrap();

    EditorWorkspace::destroy(obj->modal);
    EditorWorkspace::destroy(obj->backdrop);

    heap_delete<EditorUIModalObj>(obj);
}

void EditorUIModal::on_imgui(float delta, const Vec2& screenSize)
{
    Rect modalRect = mObj->backdrop.get_rect();
    if (Vec2(modalRect.w, modalRect.h) != screenSize)
        mObj->backdrop.set_rect(Rect(0.0f, 0.0f, screenSize.x, screenSize.y));
}

} // namespace LD