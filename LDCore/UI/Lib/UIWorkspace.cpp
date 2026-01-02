#include <Ludens/Profiler/Profiler.h>
#include <Ludens/UI/UIWorkspace.h>

#include "UIObj.h"

namespace LD {

UIWorkspaceObj::~UIWorkspaceObj()
{
    for (UIWindowObj* window : windows)
        heap_delete<UIWindowObj>(window);
}

void UIWorkspaceObj::pre_update()
{
    for (UIWindowObj* window : deferredWindowDestruction)
    {
        heap_delete<UIWindowObj>(window);

        windows.erase(std::remove(windows.begin(), windows.end(), window));
    }

    deferredWindowDestruction.clear();
}

void UIWorkspaceObj::update(float delta)
{
    for (UIWindowObj* window : windows)
    {
        if (window->cb.onUpdate)
            window->cb.onUpdate({window}, delta);

        // updates all widgets within window
        window->update(delta);
    }
}

void UIWorkspaceObj::layout()
{
    LD_PROFILE_SCOPE;

    for (UIWindowObj* window : windows)
    {
        ui_layout(window);
    }
}

//
// Public API
//

void UIWorkspace::render(ScreenRenderComponent& renderer)
{
    LD_PROFILE_SCOPE;

    for (UIWindowObj* windowObj : mObj->windows)
    {
        UIWindow(windowObj).render(renderer);
    }
}

void UIWorkspace::raise()
{
    mObj->layer->raise_workspace(mObj);
}

UIWindow UIWorkspace::create_window(UIAreaID areaID, const UILayoutInfo& layoutI, const UIWindowInfo& windowI, void* user)
{
    UIWorkspaceNode* node = mObj->partition.get_node(areaID);
    if (!node || !node->isLeaf)
        return {};

    if (node->window)
        destroy_window(node->window);

    UIContextObj* ctx = mObj->layer->ctx;

    UIWindowObj* windowObj = heap_new<UIWindowObj>(MEMORY_USAGE_UI);
    windowObj->layout.info = layoutI;
    windowObj->user = user;
    windowObj->type = UI_WIDGET_WINDOW;
    windowObj->window = windowObj;
    windowObj->node = {windowObj};
    windowObj->flags = 0;
    windowObj->theme = ctx->theme;
    windowObj->space = mObj;

    if (windowI.name)
        windowObj->name = std::string(windowI.name);

    if (windowI.hidden)
        windowObj->flags |= UI_WIDGET_FLAG_HIDDEN_BIT;

    if (windowI.drawWithScissor)
        windowObj->flags |= UI_WIDGET_FLAG_DRAW_WITH_SCISSOR_BIT;

    if (windowI.defaultMouseControls)
        windowObj->cb.onDrag = UIWindowObj::on_drag;

    mObj->windows.push_back(windowObj);
    node->window = UIWindow(windowObj);

    node->window.set_pos(node->area.get_pos());
    node->window.set_size(node->area.get_size());

    return node->window;
}

void UIWorkspace::destroy_window(UIWindow window)
{
    UIContextObj* ctx = mObj->layer->ctx;
    UIWindowObj* obj = (UIWindowObj*)window.unwrap();

    // TODO: mObj->invalidate_refs(window.unwrap()); // remove window reference in UIWorkspaceNode
    ctx->invalidate_refs((UIWidgetObj*)window.unwrap());

    // window destruction is deferred so we don't modify
    // window hierarchy while iterating it.
    mObj->deferredWindowDestruction.insert(obj);
}

void UIWorkspace::get_windows(Vector<UIWindow>& windows)
{
    windows.resize(mObj->windows.size());

    for (size_t i = 0; i < mObj->windows.size(); i++)
        windows[i] = UIWindow(mObj->windows[i]);
}

UIAreaID UIWorkspace::get_root_id()
{
    return mObj->partition.get_root_id();
}

Rect UIWorkspace::get_root_rect()
{
    UIWorkspaceNode* root = mObj->partition.get_node(mObj->partition.get_root_id());

    return root->area;
}

UIWindow UIWorkspace::get_area_window(UIAreaID areaID)
{
    UIWorkspaceNode* node = mObj->partition.get_node(areaID);

    if (!node || !node->isLeaf)
        return {};

    return node->window;
}

UIAreaID UIWorkspace::split_right(UIAreaID areaID, float ratio)
{
    return mObj->partition.split_right(areaID, ratio, mObj->splitGap);
}

UIAreaID UIWorkspace::split_bottom(UIAreaID areaID, float ratio)
{
    return mObj->partition.split_bottom(areaID, ratio, mObj->splitGap);
}

} // namespace LD