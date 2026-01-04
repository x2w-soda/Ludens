#include <Ludens/Profiler/Profiler.h>
#include <Ludens/UI/UILayer.h>

#include "UIObj.h"

namespace LD {

UILayerObj::~UILayerObj()
{
    for (UIWorkspaceObj* space : workspaces)
        heap_delete<UIWorkspaceObj>(space);
}

void UILayerObj::layout()
{
    LD_PROFILE_SCOPE;

    for (UIWorkspaceObj* space : workspaces)
    {
        space->layout();
    }
}

void UILayerObj::pre_update()
{
    for (UIWorkspaceObj* space : workspaces)
    {
        // removes windows from workspaces
        space->pre_update();
    }

    for (UIWorkspaceObj* space : deferredWorkspaceDestruction)
    {
        heap_delete<UIWorkspaceObj>(space);

        workspaces.erase(std::remove(workspaces.begin(), workspaces.end(), space));
    }

    deferredWorkspaceDestruction.clear();
}

void UILayerObj::update(float delta)
{
    for (UIWorkspaceObj* space : workspaces)
    {
        space->update(delta);
    }
}

void UILayerObj::raise_workspace(UIWorkspaceObj* obj)
{
    if (!obj)
        return;

    workspaces.erase(std::remove(workspaces.begin(), workspaces.end(), obj));
    workspaces.push_back(obj);
}

//
// Public API
//

void UILayer::layout()
{
    mObj->layout();
}

void UILayer::render(ScreenRenderComponent& renderer)
{
    for (UIWorkspaceObj* space : mObj->workspaces)
    {
        UIWorkspace(space).render(renderer);
    }
}

void UILayer::raise()
{
    mObj->ctx->raise_layer(mObj);
}

UIWorkspace UILayer::create_workspace(const Rect& area)
{
    UIWorkspaceObj* obj = heap_new<UIWorkspaceObj>(MEMORY_USAGE_UI, area);
    obj->layer = mObj;
    obj->id = ++mObj->workspaceIDCounter;

    mObj->workspaces.push_back(obj);

    return UIWorkspace(obj);
}

void UILayer::destroy_workspace(UIWorkspace workspace)
{
    if (!workspace)
        return;

    mObj->deferredWorkspaceDestruction.insert(workspace.unwrap());
}

} // namespace LD