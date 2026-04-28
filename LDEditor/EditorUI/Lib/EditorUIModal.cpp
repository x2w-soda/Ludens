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
    Vec2 screenSize;
    std::string layerName;
    bool isVisible = false;

    void prepare_modal_workspace(EditorWindowType windowType);
};

void EditorUIModalObj::prepare_modal_workspace(EditorWindowType windowType)
{
    if (modalW && modalW.type() != windowType)
    {
        EditorWorkspace::destroy(modalWS);
        modalWS = {};
        modalW = {};
    }

    if (!modalWS)
    {
        Vec2 sizeHint = EditorWorkspace::get_window_size_hint(windowType, screenSize);

        EditorWorkspaceInfo spaceI{};
        spaceI.ctx = ctx;
        spaceI.uiContextName = EDITOR_UI_CONTEXT_NAME;
        spaceI.uiLayerName = layerName.c_str();
        spaceI.isFloat = true;
        spaceI.isVisible = isVisible;
        spaceI.rootRect = Rect((screenSize.x - sizeHint.x) / 2.0f, (screenSize.y - sizeHint.y) / 2.0f, sizeHint.x, sizeHint.y);
        modalWS = EditorWorkspace::create(spaceI);
        modalW = modalWS.create_window(modalWS.get_root_id(), windowType);
    }

    LD_ASSERT(modalW && modalW.type() == windowType);
}

EditorUIModal EditorUIModal::create(const EditorUIModalInfo& modalI)
{
    LD_PROFILE_SCOPE;

    auto* obj = heap_new<EditorUIModalObj>(MEMORY_USAGE_UI);
    obj->ctx = modalI.ctx;
    obj->layerName = modalI.layerName;
    obj->isVisible = modalI.isVisible;
    obj->screenSize = modalI.screenSize;

    EditorWorkspaceInfo spaceI{};
    spaceI.ctx = obj->ctx;
    spaceI.uiContextName = EDITOR_UI_CONTEXT_NAME;
    spaceI.uiLayerName = "EDITOR_UI_LAYER_MODAL_BACKDROP";
    spaceI.isFloat = false;
    spaceI.isVisible = obj->isVisible;
    spaceI.rootRect = Rect(0.0f, 0.0f, modalI.screenSize.x, modalI.screenSize.y);
    spaceI.rootColor = 0x10101070;
    obj->backdropWS = EditorWorkspace::create(spaceI);

    obj->prepare_modal_workspace(EDITOR_WINDOW_PROJECT);

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
    mObj->screenSize = tick.screenSize;

    Rect modalRect = mObj->backdropWS.get_rect();
    if (Vec2(modalRect.w, modalRect.h) != mObj->screenSize)
        mObj->backdropWS.set_rect(Rect(0.0f, 0.0f, mObj->screenSize.x, mObj->screenSize.y));

    mObj->backdropWS.pre_update(tick);

    if (mObj->modalWS)
        mObj->modalWS.pre_update(tick);
}

void EditorUIModal::update(const EditorUpdateTick& tick)
{
    mObj->backdropWS.set_visible(mObj->isVisible);
    mObj->backdropWS.update(tick);

    if (mObj->modalWS)
    {
        mObj->modalWS.set_visible(mObj->isVisible);
        mObj->modalWS.update(tick);
    }
}

void EditorUIModal::post_update()
{
    (void)mObj->backdropWS.post_update();

    if (mObj->modalWS)
    {
        (void)mObj->modalWS.post_update();

        if (mObj->modalWS.should_close())
        {
            EditorWorkspace::destroy(mObj->modalWS);
            mObj->modalWS = {};
            mObj->modalW = {};
            mObj->isVisible = false;
        }
    }
}

EditorWindow EditorUIModal::show_window(EditorWindowType type, EditorWindowMode modeHint)
{
    mObj->prepare_modal_workspace(type);
    mObj->isVisible = true;

    if (modeHint >= 0)
        mObj->modalW.set_mode(modeHint);

    return mObj->modalW;
}

void EditorUIModal::hide()
{
    mObj->isVisible = false;
}

} // namespace LD