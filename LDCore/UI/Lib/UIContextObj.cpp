#include <Ludens/DSA/Vector.h>
#include <Ludens/Event/WindowEvent.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Bitwise.h>
#include <Ludens/Header/Math/Vec2.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/RenderComponent/ScreenRenderComponent.h>
#include <Ludens/UI/UIContext.h>
#include <Ludens/UI/UIImmediate.h>

#include <algorithm>
#include <cstdint>

#include "UIObj.h"
#include "UIWidgetMeta.h"

#include "UIDebug.h"

namespace LD {

static_assert(LD::IsTrivial<UIEvent>);

static UIEvent get_local_event(UIWidgetObj* widget, const UIEvent& event)
{
    UIEvent localEvent = event;

    switch (event.type)
    {
    case UI_EVENT_MOUSE_DOWN:
    case UI_EVENT_MOUSE_UP:
    case UI_EVENT_MOUSE_POSITION:
        localEvent.mouse.position -= widget->layout.rect.get_pos();
        break;
    default:
        break;
    }

    return localEvent;
}

/// @brief Get the widget at given position in a subtree.
/// @param root The root widget to search recursively.
/// @param pos Screen position to query.
/// @return The widget at position, or null if position is out of bounds.
static UIWidgetObj* get_widget_in_subtree(UIWidgetObj* root, const Vec2& pos)
{
    if (!root->layout.rect.contains(pos))
        return nullptr;

    for (UIWidgetObj* child = root->child; child; child = child->next)
    {
        if (!child->layout.rect.contains(pos))
            continue;

        // return deepest widget in hierarhcy that qualifies
        UIWidgetObj* result = get_widget_in_subtree(child, pos);

        if (result)
            return result;
    }

    return root;
}

/// @brief Bubble up along widget hierarchy until root widget (UIWindow),
///        returns the first widget that handles the event.
static UIWidgetObj* get_event_handler(UIWidgetObj* widget, const UIEvent& event)
{
    while (widget)
    {
        UIEvent localEvent = get_local_event(widget, event);

        if (widget->cb.onEvent && widget->cb.onEvent(UIWidget(widget), localEvent))
            return widget;

        switch (event.type)
        {
        case UI_EVENT_MOUSE_POSITION:
        case UI_EVENT_MOUSE_DOWN:
        case UI_EVENT_MOUSE_UP:
        case UI_EVENT_MOUSE_DRAG:
            if (widget->flags & UI_WIDGET_FLAG_CONSUME_MOUSE_EVENT_BIT)
                return widget;
            break;
        case UI_EVENT_KEY_DOWN:
        case UI_EVENT_KEY_UP:
            if (widget->flags & UI_WIDGET_FLAG_CONSUME_KEY_EVENT_BIT)
                return widget;
            break;
        case UI_EVENT_SCROLL:
            if (widget->flags & UI_WIDGET_FLAG_CONSUME_SCROLL_EVENT_BIT)
                return widget;
            break;
        default:
            break;
        }

        widget = widget->parent;
    }

    return nullptr;
}

UIWidgetObj* UIContextObj::alloc_widget(UIWidgetType type, const UILayoutInfo& layoutI, UIWidgetObj* parent, void* storage, void* user)
{
    LD_PROFILE_SCOPE;

    LD_ASSERT(parent);
    UIWindowObj* window = parent->window;
    UIWidgetObj* obj = (UIWidgetObj*)widgetPA.allocate();
    new (obj) UIWidgetObj(type, layoutI, parent, window, storage, user);
    obj->theme = theme;

    window->widgets.push_back(obj);
    parent->append_child(obj);

    return obj;
}

void UIContextObj::free_widget(UIWidgetObj* widget)
{
    LD_PROFILE_SCOPE;

    while (widget->child)
        free_widget(widget->child);

    UIWidgetObj* parent = widget->parent;
    if (parent)
        parent->remove_child(widget);

    UIWindowObj* window = widget->window;
    size_t count = std::erase(window->widgets, widget);
    LD_ASSERT(count == 1);

    // remove all refs to out of scope widget
    invalidate_refs(widget);

    widget->~UIWidgetObj();
    widgetPA.free(widget);
}

/// @brief Get deepest widget in context.
UIWidgetObj* UIContextObj::get_widget(const Vec2& pos)
{
    for (auto layerIte = layers.rbegin(); layerIte != layers.rend(); layerIte++)
    {
        UILayerObj* layer = *layerIte;

        UIWidgetObj* widget = get_widget_in_layer(layer, pos);
        if (widget)
            return widget;
    }

    return nullptr;
}

/// @brief Get deepest widget in layer.
UIWidgetObj* UIContextObj::get_widget_in_layer(UILayerObj* layer, const Vec2& pos)
{
    for (auto spaceIt = layer->workspaces.rbegin(); spaceIt != layer->workspaces.rend(); spaceIt++)
    {
        UIWorkspaceObj* space = *spaceIt;

        Rect workspaceRect = UIWorkspace(space).get_root_rect();
        if (space->isHidden || !workspaceRect.contains(pos))
            continue;

        UIWidgetObj* widget = get_widget_in_workspace(space, pos);
        if (widget)
            return widget;
    }

    return nullptr;
}

/// @brief Get deepest widget in workspace.
UIWidgetObj* UIContextObj::get_widget_in_workspace(UIWorkspaceObj* space, const Vec2& pos)
{
    for (auto windowIt = space->floatWindows.rbegin(); windowIt != space->floatWindows.rend(); windowIt++)
    {
        UIWindowObj* window = *windowIt;

        if (!window->layout.rect.contains(pos) || (window->flags & UI_WIDGET_FLAG_HIDDEN_BIT))
            continue;

        return get_widget_in_subtree((UIWidgetObj*)window, pos);
    }

    for (auto windowIt = space->nodeWindows.rbegin(); windowIt != space->nodeWindows.rend(); windowIt++)
    {
        UIWindowObj* window = *windowIt;

        if (!window->layout.rect.contains(pos) || (window->flags & UI_WIDGET_FLAG_HIDDEN_BIT))
            continue;

        return get_widget_in_subtree((UIWidgetObj*)window, pos);
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

    if (removed == focusWidget)
        focusWidget = nullptr;

    if (removed == hoverWidgetLeaf)
        hoverWidgetLeaf = nullptr;

    hoverWidgets.erase(removed);
}

void UIContextObj::hover_widget(UIWidgetObj* nextHoverLeafWidget)
{
    LD_PROFILE_SCOPE;

    HashSet<UIWidgetObj*> nextHoverWidgets;

    for (UIWidgetObj* widget = nextHoverLeafWidget; widget; widget = widget->parent)
        nextHoverWidgets.insert(widget);

    for (UIWidgetObj* widget : hoverWidgets)
    {
        if (!nextHoverWidgets.contains(widget))
        {
            UIEvent event{};
            event.type = UI_EVENT_MOUSE_LEAVE;
            if (widget->cb.onEvent)
                widget->cb.onEvent(UIWidget(widget), event);
            if (onEvent)
                onEvent(UIWidget(widget), event, user);
        }
    }

    for (UIWidgetObj* widget : nextHoverWidgets)
    {
        if (!hoverWidgets.contains(widget))
        {
            UIEvent event{};
            event.type = UI_EVENT_MOUSE_ENTER;
            if (widget->cb.onEvent)
                widget->cb.onEvent(UIWidget(widget), event);
            if (onEvent)
                onEvent(UIWidget(widget), event, user);
        }
    }

    hoverWidgetLeaf = nextHoverLeafWidget;
    hoverWidgets = std::move(nextHoverWidgets);
}

void UIContextObj::focus_widget(UIWidgetObj* nextFocusWidget)
{
    LD_PROFILE_SCOPE;

    // leaving focus state
    if (focusWidget && focusWidget != nextFocusWidget)
    {
        UIEvent event{};
        event.type = UI_EVENT_FOCUS_LEAVE;

        // signal widget user
        if (focusWidget->cb.onEvent)
            focusWidget->cb.onEvent(UIWidget(focusWidget), event);

        // signal context user
        if (onEvent)
            onEvent(UIWidget(focusWidget), event, user);
    }

    focusWidget = nextFocusWidget;

    // entering focus state
    if (focusWidget)
    {
        UIEvent event{};
        event.type = UI_EVENT_FOCUS_ENTER;

        // signal widget user
        if (focusWidget->cb.onEvent)
            focusWidget->cb.onEvent(UIWidget(focusWidget), event);

        // signal context user
        if (onEvent)
            onEvent(UIWidget(focusWidget), event, user);
    }
}

bool UIContextObj::input_mouse_position(const UIEvent& event)
{
    LD_PROFILE_SCOPE;
    LD_ASSERT(event.type == UI_EVENT_MOUSE_POSITION);

    cursorPos = event.mouse.position;

    if (dragWidget)
    {
        UIEvent dragEvent{};
        dragEvent.type = UI_EVENT_MOUSE_DRAG;
        dragEvent.drag.position = cursorPos;
        dragEvent.drag.button = dragMouseButton;
        dragEvent.drag.begin = false;

        if (dragWidget->cb.onEvent)
            dragWidget->cb.onEvent(UIWidget(dragWidget), dragEvent);

        if (onEvent)
            onEvent(UIWidget(dragWidget), dragEvent, user);
    }

    UIWidgetObj* nextLeaf = get_widget(cursorPos);
    hover_widget(nextLeaf);

    return true;
}

bool UIContextObj::input_mouse_down(const UIEvent& event)
{
    LD_PROFILE_SCOPE;
    LD_ASSERT(event.type == UI_EVENT_MOUSE_DOWN);

    UIWidgetObj* widget = get_widget(cursorPos);
    if (!widget)
    {
        pressWidget = nullptr;
        focus_widget(nullptr);
        return false;
    }

    // update pressed widget
    {
        pressWidget = get_event_handler(widget, event);

        if (!pressWidget)
        {
            focus_widget(nullptr);
            return false;
        }

        if (onEvent)
            onEvent(UIWidget(pressWidget), get_local_event(pressWidget, event), user);
    }

    // begin drag widget
    {
        UIEvent dragEvent{};
        dragEvent.type = UI_EVENT_MOUSE_DRAG;
        dragEvent.drag.position = dragStartPos = cursorPos;
        dragEvent.drag.button = dragMouseButton = event.mouse.button;
        dragEvent.drag.begin = true;
        dragWidget = get_event_handler(widget, dragEvent);

        if (dragWidget && onEvent)
            onEvent(UIWidget(dragWidget), dragEvent, user);
    }

    // update focused widget
    {
        UIWidgetObj* nextFocusWidget = nullptr;

        while (widget)
        {
            if (widget->flags & UI_WIDGET_FLAG_FOCUSABLE_BIT)
            {
                nextFocusWidget = widget;
                break;
            }

            widget = widget->parent;
        }

        focus_widget(nextFocusWidget);
    }

    return true;
}

bool UIContextObj::input_mouse_up(const UIEvent& event)
{
    LD_PROFILE_SCOPE;
    LD_ASSERT(event.type == UI_EVENT_MOUSE_UP);

    dragWidget = nullptr;
    pressWidget = nullptr; // TODO: signal pressWidget user mouse up?

    UIWidgetObj* widget = get_widget(cursorPos);
    if (!widget)
        return false;

    widget = get_event_handler(widget, event);
    if (!widget)
        return false;

    // signal context user
    if (onEvent)
        onEvent(UIWidget(widget), get_local_event(widget, event), user);

    return true;
}

bool UIContextObj::input_key(const UIEvent& event)
{
    LD_PROFILE_SCOPE;
    LD_ASSERT(event.type == UI_EVENT_KEY_DOWN || event.type == UI_EVENT_KEY_UP);

    // 1. Focused widget and its ancestor checked first.
    UIWidgetObj* widget = get_event_handler(focusWidget, event);

    // 2. Fallback to leaf hover widget and its ancestors.
    if (!widget)
        widget = get_event_handler(hoverWidgetLeaf, event);

    // 3. Global fallback among layers and workspaces.
    //    Technically we only need to start from the focus or hover widget layer...
    if (!widget)
    {
        for (auto layerIt = layers.rbegin(); !widget && layerIt != layers.rend(); ++layerIt)
        {
            UILayerObj* layer = *layerIt;

            for (auto spaceIt = layer->workspaces.rbegin(); !widget && spaceIt != layer->workspaces.rend(); ++spaceIt)
            {
                UIWidgetObj* widgetInWorkspace = get_widget_in_workspace(*spaceIt, cursorPos);

                if ((widget = get_event_handler(widgetInWorkspace, event)))
                    break;
            }
        }
    }

    if (!widget)
        return false;

    // signal context user
    if (onEvent)
        onEvent(UIWidget(widget), get_local_event(widget, event), user);

    return true;
}

bool UIContextObj::input_scroll(const UIEvent& event)
{
    LD_PROFILE_SCOPE;
    LD_ASSERT(event.type == UI_EVENT_SCROLL);

    UIWidgetObj* widget = get_widget(cursorPos);
    if (!widget)
        return false;

    widget = get_event_handler(widget, event);
    if (!widget)
        return false;

    // signal context user
    if (onEvent)
        onEvent(UIWidget(widget), get_local_event(widget, event), user);

    return true;
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
    obj->fontDefault = info.font;
    obj->user = info.user;
    obj->onEvent = info.onEvent;

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

void UIContext::render(ScreenRenderComponent renderer)
{
    for (UILayerObj* layer : mObj->layers)
    {
        UILayer(layer).render(renderer);
    }
}

bool UIContext::input_key_down(KeyCode code, KeyMods mods)
{
    UIEvent event{};
    event.type = UI_EVENT_KEY_DOWN;
    event.key.code = code;
    event.key.mods = mods;
    return mObj->input_key(event);
}

bool UIContext::input_key_up(KeyCode code, KeyMods mods)
{
    UIEvent event{};
    event.type = UI_EVENT_KEY_UP;
    event.key.code = code;
    event.key.mods = mods;
    return mObj->input_key(event);
}

bool UIContext::input_mouse_position(const Vec2& pos)
{
    UIEvent event{};
    event.type = UI_EVENT_MOUSE_POSITION;
    event.mouse.position = pos;
    return mObj->input_mouse_position(event);
}

bool UIContext::input_scroll(const Vec2& offset)
{
    UIEvent event{};
    event.type = UI_EVENT_SCROLL;
    event.scroll.offset = offset;
    return mObj->input_scroll(event);
}

bool UIContext::input_mouse_down(MouseButton btn, KeyMods mods, const Vec2& pos)
{
    UIEvent event{};
    event.type = UI_EVENT_MOUSE_DOWN;
    event.mouse.button = btn;
    event.mouse.mods = mods;
    event.mouse.position = pos;
    return mObj->input_mouse_down(event);
}

bool UIContext::input_mouse_up(MouseButton btn, KeyMods mods, const Vec2& pos)
{
    UIEvent event{};
    event.type = UI_EVENT_MOUSE_UP;
    event.mouse.button = btn;
    event.mouse.mods = mods;
    event.mouse.position = pos;
    return mObj->input_mouse_up(event);
}

bool UIContext::input_window_event(const WindowEvent* event)
{
    bool isHandled = false;

    switch (event->type)
    {
    case EVENT_TYPE_WINDOW_KEY_DOWN:
        isHandled = input_key_down(static_cast<const WindowKeyDownEvent*>(event)->code,
                                   static_cast<const WindowKeyDownEvent*>(event)->mods);
        break;
    case EVENT_TYPE_WINDOW_KEY_UP:
        isHandled = input_key_up(static_cast<const WindowKeyUpEvent*>(event)->code,
                                 static_cast<const WindowKeyUpEvent*>(event)->mods);
        break;
    case EVENT_TYPE_WINDOW_MOUSE_DOWN:
        isHandled = input_mouse_down(
            static_cast<const WindowMouseDownEvent*>(event)->button,
            static_cast<const WindowMouseDownEvent*>(event)->mods,
            mObj->cursorPos);
        break;
    case EVENT_TYPE_WINDOW_MOUSE_UP:
        isHandled = input_mouse_up(
            static_cast<const WindowMouseUpEvent*>(event)->button,
            static_cast<const WindowMouseUpEvent*>(event)->mods,
            mObj->cursorPos);
        break;
    case EVENT_TYPE_WINDOW_MOUSE_POSITION:
        isHandled = input_mouse_position(Vec2(static_cast<const WindowMousePositionEvent*>(event)->xpos,
                                              static_cast<const WindowMousePositionEvent*>(event)->ypos));
        break;
    case EVENT_TYPE_WINDOW_SCROLL:
        isHandled = input_scroll(Vec2(static_cast<const WindowScrollEvent*>(event)->xoffset,
                                      static_cast<const WindowScrollEvent*>(event)->yoffset));
        break;
    default:
        break;
    }

    return isHandled;
}

void UIContext::set_user(void* user)
{
    mObj->user = user;
}

void UIContext::set_on_event(void (*onEvent)(UIWidget widget, const UIEvent& event, void* user))
{
    mObj->onEvent = onEvent;
}

std::string UIContext::print()
{
    UIDebug debug{};

    debug.print_context(mObj);

    return debug.ss.str();
}

} // namespace LD