#include <Ludens/Profiler/Profiler.h>
#include <Ludens/WindowRegistry/Input.h>
#include <LudensEditor/EditorUI/EditorUIModal.h>
#include <LudensEditor/EditorUI/EditorWorkspace.h>

#include "EditorUIDef.h"

namespace LD {

struct EditorUIModalObj
{
    EditorContext ctx;
    EditorWorkspace backdropWS;
    EditorWorkspace modalWS;
    EditorWindow modalW;
    std::string layerName;
    bool isVisible = false;
};

EditorUIModal EditorUIModal::create(const EditorUIModalInfo& modalI)
{
    LD_PROFILE_SCOPE;

    auto* obj = heap_new<EditorUIModalObj>(MEMORY_USAGE_UI);
    obj->ctx = modalI.ctx;
    obj->layerName = modalI.layerName;
    obj->isVisible = false;

    EditorWorkspaceInfo spaceI{};
    spaceI.ctx = obj->ctx;
    spaceI.uiContextName = EDITOR_UI_CONTEXT_NAME;
    spaceI.uiLayerName = "EDITOR_UI_LAYER_MODAL_BACKDROP";
    spaceI.isFloat = false;
    spaceI.isVisible = obj->isVisible;
    spaceI.rootRect = Rect(0.0f, 0.0f, modalI.screenSize.x, modalI.screenSize.y);
    spaceI.rootColor = 0x10101070;
    obj->backdropWS = EditorWorkspace::create(spaceI);

    const float modalW = 600.0f;
    const float modalH = 700.0f;
    spaceI.uiLayerName = modalI.layerName;
    spaceI.isFloat = true;
    spaceI.isVisible = obj->isVisible;
    spaceI.rootRect = Rect((modalI.screenSize.x - modalW) / 2.0f, (modalI.screenSize.y - modalH) / 2.0f, modalW, modalH);
    spaceI.rootColor = 0;
    obj->modalWS = EditorWorkspace::create(spaceI);
    obj->modalW = obj->modalWS.create_window(obj->modalWS.get_root_id(), EDITOR_WINDOW_PROJECT);

    return EditorUIModal(obj);
}

void EditorUIModal::destroy(EditorUIModal modal)
{
    LD_PROFILE_SCOPE;

    auto* obj = modal.unwrap();

    EditorWorkspace::destroy(obj->modalWS);
    EditorWorkspace::destroy(obj->backdropWS);

    heap_delete<EditorUIModalObj>(obj);
}

void EditorUIModal::pre_update(const EditorUpdateTick& tick)
{
    Rect modalRect = mObj->backdropWS.get_rect();
    if (Vec2(modalRect.w, modalRect.h) != tick.screenSize)
        mObj->backdropWS.set_rect(Rect(0.0f, 0.0f, tick.screenSize.x, tick.screenSize.y));
}

void EditorUIModal::update(const EditorUpdateTick& tick)
{
    mObj->backdropWS.set_visible(mObj->isVisible);
    mObj->backdropWS.update(tick);

    mObj->modalWS.set_visible(mObj->isVisible);
    mObj->modalWS.update(tick);
}

void EditorUIModal::post_update()
{
    mObj->backdropWS.post_update();
    
    Vector<EditorAreaID> destroyed = mObj->modalWS.post_update();
    if (!destroyed.empty() && destroyed.front() == mObj->modalWS.get_root_id())
    {
        mObj->modalW = {};
        mObj->isVisible = false;
    }
}

void EditorUIModal::set_visible(bool isVisible)
{
    mObj->isVisible = isVisible;
}

EditorWindow EditorUIModal::set_window(EditorWindowType type)
{
    if (mObj->modalW && mObj->modalW.type() == type)
        return mObj->modalW;

    return mObj->modalW = mObj->modalWS.create_window(mObj->modalWS.get_root_id(), type);
}

} // namespace LD