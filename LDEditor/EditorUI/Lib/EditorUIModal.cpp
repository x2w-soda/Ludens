#include <LudensEditor/EditorUI/EditorUIModal.h>
#include <LudensEditor/EditorUI/EditorWorkspace.h>

#include <Ludens/WindowRegistry/Input.h>

namespace LD {

struct EditorUIModalObj
{
    EditorContext ctx;
    EditorWorkspace workspace;
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
    spaceI.uiLayerName = obj->layerName;
    spaceI.isFloat = true;
    spaceI.isVisible = obj->isVisible;
    spaceI.rootRect = Rect(0.0f, 0.0f, modalI.screenSize.x, modalI.screenSize.y);
    spaceI.rootColor = 0x10101070;
    obj->workspace = EditorWorkspace::create(spaceI);

    return EditorUIModal(obj);
}

void EditorUIModal::destroy(EditorUIModal modal)
{
    auto* obj = modal.unwrap();

    EditorWorkspace::destroy(obj->workspace);

    heap_delete<EditorUIModalObj>(obj);
}

void EditorUIModal::on_imgui(float delta, const Vec2& screenSize)
{
    Rect modalRect = mObj->workspace.get_rect();
    if (Vec2(modalRect.w, modalRect.h) != screenSize)
        mObj->workspace.set_rect(Rect(0.0f, 0.0f, screenSize.x, screenSize.y));
}

} // namespace LD