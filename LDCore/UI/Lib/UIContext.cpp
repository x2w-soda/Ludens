#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Bitwise.h>
#include <Ludens/Header/Math/Vec2.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/RenderComponent/ScreenRenderComponent.h>
#include <Ludens/UI/UIContext.h>

#include <algorithm>
#include <cstdint>

#include "UIObj.h"

namespace LD {

enum UIWidgetFilterBit : int
{
    WIDGET_FILTER_KEY_BIT = LD_BIT(0),
    WIDGET_FILTER_MOUSE_BIT = LD_BIT(1),
    WIDGET_FILTER_HOVER_BIT = LD_BIT(2),
    WIDGET_FILTER_DRAG_BIT = LD_BIT(3),
    WIDGET_FILTER_SCROLL_BIT = LD_BIT(4),
};

/// @brief Get the widget at given position in a subtree.
/// @param root The root widget to search recursively.
/// @param pos Screen position to query.
/// @param filter Filter widgets with certain callbacks.
/// @return The widget at position, or null if position is out of bounds.
static UIWidgetObj* get_widget_at_pos(UIWidgetObj* root, const Vec2& pos, int filter)
{
    if (!root->layout.rect.contains(pos))
        return nullptr;

    if (root->flags & UI_WIDGET_FLAG_BLOCK_INPUT_BIT)
        return nullptr; // prevent subtree from being scanned

    bool isQualified = filter == 0;

    if (filter & WIDGET_FILTER_KEY_BIT)
        isQualified = isQualified || root->cb.onKey;
    if (filter & WIDGET_FILTER_MOUSE_BIT)
        isQualified = isQualified || root->cb.onMouse;
    if (filter & WIDGET_FILTER_HOVER_BIT)
        isQualified = isQualified || root->cb.onHover;
    if (filter & WIDGET_FILTER_DRAG_BIT)
        isQualified = isQualified || root->cb.onDrag;
    if (filter & WIDGET_FILTER_SCROLL_BIT)
        isQualified = isQualified || root->cb.onScroll;

    for (UIWidgetObj* child = root->child; child; child = child->next)
    {
        if (!child->layout.rect.contains(pos))
            continue;

        // return deepest widget in hierarhcy that qualifies
        UIWidgetObj* result = get_widget_at_pos(child, pos, filter);

        if (result)
            return result;
    }

    return isQualified ? root : nullptr;
}

UIWidgetObj* UIContextObj::alloc_widget(UIWidgetType type, const UILayoutInfo& layoutI, UIWidgetObj* parent, void* user)
{
    UIWindowObj* window = parent->window;

    UIWidgetObj* obj = (UIWidgetObj*)widgetPA.allocate();
    memset(obj, 0, sizeof(UIWidgetObj));
    obj->layout.info = layoutI;
    obj->type = type;
    obj->parent = parent;
    obj->window = window;
    obj->user = user;
    obj->node = {obj};
    obj->theme = window->ctx()->theme;
    obj->scrollOffset = Vec2(0.0f);
    obj->flags = 0;

    window->widgets.push_back(obj);
    parent->append_child(obj);

    return obj;
}

void UIContextObj::free_widget(UIWidgetObj* widget)
{
    while (widget->child)
        free_widget(widget->child);

    UIWidgetObj* parent = widget->parent;
    if (parent)
        parent->remove_child(widget);

    UIWindowObj* window = widget->window;
    size_t count = std::erase(window->widgets, widget);
    LD_ASSERT(count == 1);

    invalidate_refs(widget); // remove all refs to out of scope widget
    ui_obj_cleanup(widget);  // polymorphic cleanup

    widgetPA.free(widget);
}

UIWidgetObj* UIContextObj::get_widget(const Vec2& pos, int filter)
{
    for (auto layerIte = layers.rbegin(); layerIte != layers.rend(); layerIte++)
    {
        UILayerObj* layer = *layerIte;

        for (auto spaceIt = layer->workspaces.rbegin(); spaceIt != layer->workspaces.rend(); spaceIt++)
        {
            UIWorkspaceObj* space = *spaceIt;

            Rect workspaceRect = UIWorkspace(space).get_root_rect();
            if (space->isHidden || !workspaceRect.contains(pos))
                continue;

            // test floating windows in workspace before docked ones
            for (auto windowIte = space->floatWindows.rbegin(); windowIte != space->floatWindows.rend(); windowIte++)
            {
                UIWindowObj* window = *windowIte;

                if (!window->layout.rect.contains(pos) || (window->flags & UI_WIDGET_FLAG_HIDDEN_BIT))
                    continue;

                if (window->flags & UI_WIDGET_FLAG_BLOCK_INPUT_BIT)
                    return nullptr;

                return get_widget_at_pos((UIWidgetObj*)window, pos, filter);
            }

            for (auto windowIte = space->nodeWindows.rbegin(); windowIte != space->nodeWindows.rend(); windowIte++)
            {
                UIWindowObj* window = *windowIte;

                if (!window->layout.rect.contains(pos) || (window->flags & UI_WIDGET_FLAG_HIDDEN_BIT))
                    continue;

                if (window->flags & UI_WIDGET_FLAG_BLOCK_INPUT_BIT)
                    return nullptr;

                return get_widget_at_pos((UIWidgetObj*)window, pos, filter);
            }
        }
    }

    return nullptr;
}

void UIContextObj::pre_update()
{
    for (UILayerObj* layer : layers)
    {
        // removes workspaces from layers
        layer->pre_update();
    }

    for (UILayerObj* layer : deferredLayerDestruction)
    {
        heap_delete<UILayerObj>(layer);

        layers.erase(std::remove(layers.begin(), layers.end(), layer));
    }

    deferredLayerDestruction.clear();
}

void UIContextObj::raise_layer(UILayerObj* obj)
{
    if (!obj)
        return;

    layers.erase(std::remove(layers.begin(), layers.end(), obj));
    layers.push_back(obj);
}

UILayerObj* UIContextObj::get_or_create_layer(const char* name)
{
    UILayerObj* obj = get_layer(name);

    if (obj)
        return obj;

    obj = heap_new<UILayerObj>(MEMORY_USAGE_UI);
    obj->name = std::string(name);
    obj->ctx = this;
    layers.push_back(obj);

    return obj;
}

UILayerObj* UIContextObj::get_layer(const char* name)
{
    // just linear probe
    for (UILayerObj* layer : layers)
    {
        if (layer->name == name)
            return layer;
    }

    return nullptr;
}

void UIContextObj::invalidate_refs(UIWidgetObj* removed)
{
    if (removed == dragWidget)
        dragWidget = nullptr;

    if (removed == pressWidget)
        pressWidget = nullptr;

    if (removed == cursorWidget)
        cursorWidget = nullptr;
}

void UIContextObj::input_mouse_position(const Vec2& pos)
{
    LD_PROFILE_SCOPE;

    cursorPos = pos;

    if (dragWidget)
    {
        UIWidgetObj* de = dragWidget;
        de->cb.onDrag({de}, dragMouseButton, cursorPos, false);
    }

    UIWidgetObj* prev = cursorWidget;
    UIWidgetObj* next = get_widget(pos, WIDGET_FILTER_HOVER_BIT);

    if (next)
    {
        if (next != prev && prev && prev->cb.onHover)
            prev->cb.onHover({prev}, UI_MOUSE_LEAVE);

        if (next != prev && next->cb.onHover)
            next->cb.onHover({next}, UI_MOUSE_ENTER);

        cursorWidget = next;
        return;
    }

    if (prev && prev->cb.onHover)
        prev->cb.onHover({prev}, UI_MOUSE_LEAVE);

    cursorWidget = nullptr;
}

void UIContextObj::input_mouse_down(MouseButton btn)
{
    UIWidgetObj* widget = get_widget(cursorPos, WIDGET_FILTER_MOUSE_BIT | WIDGET_FILTER_DRAG_BIT);

    if (!widget)
        return;

    bool blockInput = (widget->flags & UI_WIDGET_FLAG_BLOCK_INPUT_BIT);

    if (!blockInput && widget->cb.onDrag)
    {
        dragStartPos = cursorPos;
        dragWidget = widget;
        dragMouseButton = btn;

        widget->cb.onDrag({widget}, btn, cursorPos, true);
    }

    if (!blockInput && widget->cb.onMouse)
    {
        Vec2 localPos = cursorPos - widget->layout.rect.get_pos();
        widget->cb.onMouse({widget}, localPos, btn, UI_MOUSE_DOWN);
        pressWidget = widget;
    }
}

void UIContextObj::input_mouse_up(MouseButton btn)
{
    dragWidget = nullptr;
    pressWidget = nullptr;

    UIWidgetObj* widget = get_widget(cursorPos, WIDGET_FILTER_MOUSE_BIT);

    if (!widget)
        return;

    bool blockInput = (widget->flags & UI_WIDGET_FLAG_BLOCK_INPUT_BIT);

    if (!blockInput && widget->cb.onMouse)
    {
        Vec2 localPos = cursorPos - widget->layout.rect.get_pos();
        widget->cb.onMouse({widget}, localPos, btn, UI_MOUSE_UP);
    }
}

void UIContextObj::input_key_down(KeyCode key)
{
    UIWidgetObj* widget = get_widget(cursorPos, WIDGET_FILTER_KEY_BIT);

    if (!widget)
        return;

    bool blockInput = (widget->flags & UI_WIDGET_FLAG_BLOCK_INPUT_BIT);

    if (!blockInput && widget->cb.onKey)
        widget->cb.onKey({widget}, key, UI_KEY_DOWN);
}

void UIContextObj::input_key_up(KeyCode key)
{
    UIWidgetObj* widget = get_widget(cursorPos, WIDGET_FILTER_KEY_BIT);

    if (!widget)
        return;

    bool blockInput = (widget->flags & UI_WIDGET_FLAG_BLOCK_INPUT_BIT);

    if (!blockInput && widget->cb.onKey)
        widget->cb.onKey({widget}, key, UI_KEY_UP);
}

void UIContextObj::input_scroll(const Vec2& offset)
{
    UIWidgetObj* widget = get_widget(cursorPos, WIDGET_FILTER_SCROLL_BIT);

    if (!widget)
        return;

    bool blockInput = (widget->flags & UI_WIDGET_FLAG_BLOCK_INPUT_BIT);

    if (!blockInput && widget->cb.onScroll)
        widget->cb.onScroll({widget}, offset);
}

//
// Public API
//

UILayer UIContext::create_layer(const char* layerName)
{
    return UILayer(mObj->get_or_create_layer(layerName));
}

void UIContext::destroy_layer(UILayer layer)
{
    if (!layer)
        return;

    mObj->deferredLayerDestruction.insert(layer.unwrap());
}

void UIContext::get_layers(Vector<UILayer>& layers)
{
    if (!mObj->deferredLayerDestruction.empty())
        mObj->pre_update();

    layers.resize(mObj->layers.size());

    for (int i = 0; i < (int)mObj->layers.size(); i++)
        layers[i] = UILayer(mObj->layers[i]);
}

UITheme UIContext::get_theme()
{
    return mObj->theme;
}

Vec2 UIContext::get_mouse_pos()
{
    return mObj->cursorPos;
}

UIContext UIContext::create(const UIContextInfo& info)
{
    LD_ASSERT(info.theme);

    UIContextObj* obj = heap_new<UIContextObj>(MEMORY_USAGE_UI);
    obj->fontAtlas = info.fontAtlas;
    obj->fontAtlasImage = info.fontAtlasImage;

    PoolAllocatorInfo paI{};
    paI.blockSize = sizeof(UIWidgetObj);
    paI.isMultiPage = true;
    paI.pageSize = 64; // widgets per memory page
    paI.usage = MEMORY_USAGE_UI;
    obj->widgetPA = PoolAllocator::create(paI);
    obj->theme = info.theme;

    return UIContext(obj);
}

void UIContext::destroy(UIContext ctx)
{
    UIContextObj* obj = ctx.unwrap();

    for (UILayerObj* layer : obj->layers)
        obj->deferredLayerDestruction.insert(layer);

    obj->pre_update();
    LD_ASSERT(obj->layers.empty());

    PoolAllocator::destroy(obj->widgetPA);
    heap_delete<UIContextObj>(obj);
}

void UIContext::update(float delta)
{
    LD_PROFILE_SCOPE;

    mObj->pre_update();

    for (UILayerObj* layer : mObj->layers)
    {
        layer->layout();
        layer->update(delta);
    }
}

bool UIContext::on_event(const Event* event)
{
    switch (event->type)
    {
    case EVENT_TYPE_KEY_DOWN:
        mObj->input_key_down(static_cast<const KeyDownEvent*>(event)->key);
        break;
    case EVENT_TYPE_KEY_UP:
        mObj->input_key_up(static_cast<const KeyUpEvent*>(event)->key);
        break;
    case EVENT_TYPE_MOUSE_MOTION:
        mObj->input_mouse_position(Vec2(static_cast<const MouseMotionEvent*>(event)->xpos,
                                        static_cast<const MouseMotionEvent*>(event)->ypos));
        break;
    case EVENT_TYPE_MOUSE_DOWN:
        mObj->input_mouse_down(static_cast<const MouseDownEvent*>(event)->button);
        break;
    case EVENT_TYPE_MOUSE_UP:
        mObj->input_mouse_up(static_cast<const MouseUpEvent*>(event)->button);
        break;
    case EVENT_TYPE_SCROLL:
        mObj->input_scroll(Vec2(static_cast<const ScrollEvent*>(event)->xoffset,
                                static_cast<const ScrollEvent*>(event)->yoffset));
        break;
    default: // does not trigger any input
        return false;
    }

    return true;
}

} // namespace LD