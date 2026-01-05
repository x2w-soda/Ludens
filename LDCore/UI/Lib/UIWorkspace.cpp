#include <Ludens/Profiler/Profiler.h>
#include <Ludens/UI/UIWorkspace.h>

#include "UIObj.h"

namespace LD {

UIWorkspaceObj::~UIWorkspaceObj()
{
    for (UIWindowObj* window : nodeWindows)
        heap_delete<UIWindowObj>(window);

    for (UIWindowObj* window : floatWindows)
        heap_delete<UIWindowObj>(window);
}

UIWindowObj* UIWorkspaceObj::create_window(const UILayoutInfo& layoutI, const UIWindowInfo& windowI, void* user)
{
    UIWindowObj* windowObj = heap_new<UIWindowObj>(MEMORY_USAGE_UI);
    windowObj->layout.info = layoutI;
    windowObj->user = user;
    windowObj->type = UI_WIDGET_WINDOW;
    windowObj->window = windowObj;
    windowObj->node = {windowObj};
    windowObj->flags = 0;
    windowObj->theme = layer->ctx->theme;
    windowObj->space = this;
    windowObj->id = ++windowIDCounter;

    if (windowI.name)
        windowObj->debugName = std::string(windowI.name);

    if (windowI.hidden)
        windowObj->flags |= UI_WIDGET_FLAG_HIDDEN_BIT;

    if (windowI.drawWithScissor)
        windowObj->flags |= UI_WIDGET_FLAG_DRAW_WITH_SCISSOR_BIT;

    if (windowI.defaultMouseControls)
        windowObj->cb.onDrag = UIWindowObj::on_drag;

    return windowObj;
}

Hash64 UIWorkspaceObj::get_hash() const
{
    uint64_t hash = Hash64(layer->name.c_str());
    hash_combine(hash, id); // unique within layer

    return hash;
}

void UIWorkspaceObj::pre_update()
{
    for (UIWindowObj* window : deferredWindowDestruction)
    {
        heap_delete<UIWindowObj>(window);

        auto ite = std::find(nodeWindows.begin(), nodeWindows.end(), window);
        if (ite != nodeWindows.end())
            nodeWindows.erase(ite);
        else
            floatWindows.erase(std::remove(floatWindows.begin(), floatWindows.end(), window));
    }

    deferredWindowDestruction.clear();
}

void UIWorkspaceObj::update(float delta)
{
    for (UIWindowObj* window : nodeWindows)
    {
        if (window->cb.onUpdate)
            window->cb.onUpdate({window}, delta);

        // updates all widgets within window
        window->update(delta);
    }

    for (UIWindowObj* window : floatWindows)
    {
        if (window->cb.onUpdate)
            window->cb.onUpdate({window}, delta);

        window->update(delta);
    }
}

void UIWorkspaceObj::layout()
{
    LD_PROFILE_SCOPE;

    for (UIWindowObj* window : nodeWindows)
    {
        ui_layout(window);
    }

    for (UIWindowObj* window : floatWindows)
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

    if (mObj->isHidden)
        return;

    for (UIWindowObj* windowObj : mObj->nodeWindows)
    {
        UIWindow(windowObj).render(renderer);
    }

    for (UIWindowObj* windowObj : mObj->floatWindows)
    {
        UIWindow(windowObj).render(renderer);
    }
}

void UIWorkspace::raise()
{
    mObj->layer->raise_workspace(mObj);
}

void UIWorkspace::set_visible(bool isVisible)
{
    mObj->isHidden = !isVisible;
}

void UIWorkspace::set_rect(const Rect& rect)
{
    mObj->partition.set_root_rect(rect);

    mObj->partition.visit_leaves(mObj->partition.get_root_id(), [](UIWorkspaceNode* node) {
        node->window.set_rect(node->rect);

        auto* windowObj = (UIWindowObj*)node->window.unwrap();
        if (windowObj->onResize)
            windowObj->onResize(node->window, node->rect.get_size());
    });
}

void UIWorkspace::set_pos(const Vec2& pos)
{
    mObj->partition.set_root_pos(pos);

    mObj->partition.visit_leaves(mObj->partition.get_root_id(), [](UIWorkspaceNode* node) {
        node->window.set_pos(node->rect.get_pos());
    });
}

UIWindow UIWorkspace::create_window(UIAreaID areaID, const UILayoutInfo& layoutI, const UIWindowInfo& windowI, void* user)
{
    UIWorkspaceNode* node = mObj->partition.get_node(areaID);
    if (!node || !node->isLeaf)
        return {};

    if (node->window)
        destroy_window(node->window);

    UIContextObj* ctx = mObj->layer->ctx;
    UIWindowObj* windowObj = mObj->create_window(layoutI, windowI, user);

    mObj->nodeWindows.push_back(windowObj);
    node->window = UIWindow(windowObj);

    // override window rect with docked area rect
    node->window.set_pos(node->rect.get_pos());
    node->window.set_size(node->rect.get_size());

    return node->window;
}

UIWindow UIWorkspace::create_window(const UILayoutInfo& layoutI, const UIWindowInfo& windowI, void* user)
{
    UIWindowObj* windowObj = mObj->create_window(layoutI, windowI, user);

    mObj->floatWindows.push_back(windowObj);

    return UIWindow(windowObj);
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

void UIWorkspace::get_docked_windows(Vector<UIWindow>& windows)
{
    windows.resize(mObj->nodeWindows.size());

    for (size_t i = 0; i < mObj->nodeWindows.size(); i++)
        windows[i] = UIWindow(mObj->nodeWindows[i]);
}

Hash64 UIWorkspace::get_hash()
{
    return mObj->get_hash();
}

UIAreaID UIWorkspace::get_root_id()
{
    return mObj->partition.get_root_id();
}

Rect UIWorkspace::get_root_rect()
{
    UIWorkspaceNode* root = mObj->partition.get_node(mObj->partition.get_root_id());

    return root->rect;
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
    return mObj->partition.split_right(areaID, ratio);
}

UIAreaID UIWorkspace::split_bottom(UIAreaID areaID, float ratio)
{
    return mObj->partition.split_bottom(areaID, ratio);
}

} // namespace LD