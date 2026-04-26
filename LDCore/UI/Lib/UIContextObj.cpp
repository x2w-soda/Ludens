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
        localEvent.mouse.position -= widget->L->rect.get_pos();
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
    if (!root->L->rect.contains(pos))
        return nullptr;

    for (UIWidgetObj* child = root->child; child; child = child->next)
    {
        if (!child->L->rect.contains(pos))
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

        if (widget_on_event(widget, localEvent))
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

void UIOverlay::startup(UIContextObj* ctx)
{
    LD_ASSERT(!layerObj);

    layerObj = heap_new<UILayerObj>(MEMORY_USAGE_UI, ctx);
    layerObj->name = std::string("OVERLAY");

    workspace = UILayer(layerObj).create_workspace(Rect());
}

void UIOverlay::cleanup()
{
    LD_ASSERT(layerObj && workspace);

    UILayer layer(layerObj);

    layer.destroy_workspace(workspace);
    layerObj->pre_update(); // force destruction

    heap_delete<UILayerObj>(layerObj);
    layerObj = nullptr;
}

void UIOverlay::pre_update()
{
    layerObj->pre_update();
}

void UIOverlay::reserve_windows(int level)
{
    if (level >= (int)stack.size())
    {
        UILayoutInfo layoutI(UISize::fit(), UISize::fit(), UI_AXIS_Y);

        stack.resize(level + 1);
        for (int i = level; i < (int)stack.size(); i++)
        {
            stack[i] = workspace.create_float_window(layoutI, {}, nullptr);
            stack[i].hide();
        }
    }
}

int UIOverlay::find_widget(UIWidgetObj* obj)
{
    if (!obj)
        return -1;

    for (size_t i = 0; i < stack.size(); i++)
    {
        if (stack[i].unwrap()->id == obj->window->id)
            return (int)i;
    }

    return -1;
}

UIWindow UIOverlay::set_level(int level)
{
    reserve_windows(level);

    for (int i = level + 1; i < (int)stack.size(); i++)
    {
        stack[i].hide();
    }

    currentLevel = level;
    if (currentLevel < 0)
        return {};

    UIWindow window = stack[currentLevel];
    window.show();

    return window;
}

UIFont UIContextObj::get_font_from_hint(TextSpanFont font)
{
    // TODO: UIContext is responsible for mapping TextSpanFont intent
    //       to actual UIFont handle, generalize for itatlic and bold fonts.
    if (font == TEXT_SPAN_FONT_MONOSPACE && fontMonospace)
        return fontMonospace;

    return fontDefault;
}

UIWidgetObj* UIContextObj::alloc_widget_obj(const UIWidgetAllocInfo& info)
{
    LD_ASSERT(info.parent);

    UIWidgetLayout* widgetL = (UIWidgetLayout*)widgetLayoutPA.allocate();
    UIWidgetUnion* widgetU = (UIWidgetUnion*)widgetUnionPA.allocate();
    UIWindowObj* window = info.parent->window;
    UIWidgetObj* obj = (UIWidgetObj*)widgetObjPA.allocate();
    new (obj) UIWidgetObj(info.type, this, widgetL, widgetU, info.parent, window, info.data, info.user);

    widget_startup(obj);

    window->widgets.push_back(obj);
    info.parent->append_child(obj);

    return obj;
}

void UIContextObj::free_widget_obj(UIWidgetObj* widget)
{
    while (widget->child)
        free_widget_obj(widget->child);

    UIWidgetObj* parent = widget->parent;
    if (parent)
        parent->remove_child(widget);

    UIWindowObj* window = widget->window;
    size_t count = std::erase(window->widgets, widget);
    LD_ASSERT(count == 1);

    // remove all refs to out of scope widget
    invalidate_refs(widget);

    UIWidgetLayout* widgetL = widget->L;
    UIWidgetUnion* widgetU = widget->U;

    widget_cleanup(widget);

    widget->~UIWidgetObj();

    widgetObjPA.free(widget);
    widgetUnionPA.free(widgetU);
    widgetLayoutPA.free(widgetL);
}

/// @brief Get deepest widget in context.
UIWidgetObj* UIContextObj::get_widget(const Vec2& pos)
{
    UIWidgetObj* widgetObj = get_widget_in_layer(overlay.layerObj, pos);
    if (widgetObj)
        return widgetObj;

    for (auto layerIt = layers.rbegin(); layerIt != layers.rend(); layerIt++)
    {
        UILayerObj* layerObj = *layerIt;

        widgetObj = get_widget_in_layer(layerObj, pos);
        if (widgetObj)
            return widgetObj;
    }

    return nullptr;
}

/// @brief Get deepest widget in layer.
UIWidgetObj* UIContextObj::get_widget_in_layer(UILayerObj* layer, const Vec2& pos)
{
    if (!layer->isVisible)
        return nullptr;

    for (auto spaceIt = layer->workspaces.rbegin(); spaceIt != layer->workspaces.rend(); spaceIt++)
    {
        UIWorkspaceObj* space = *spaceIt;

        Rect workspaceRect = UIWorkspace(space).get_root_rect();
        if (!space->isVisible || !workspaceRect.contains(pos))
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
    if (!space->isVisible)
        return nullptr;

    for (auto windowIt = space->floatWindows.rbegin(); windowIt != space->floatWindows.rend(); windowIt++)
    {
        UIWindowObj* window = *windowIt;

        if (!window->L->rect.contains(pos) || (window->flags & UI_WIDGET_FLAG_HIDDEN_BIT))
            continue;

        return get_widget_in_subtree((UIWidgetObj*)window, pos);
    }

    for (auto windowIt = space->nodeWindows.rbegin(); windowIt != space->nodeWindows.rend(); windowIt++)
    {
        UIWindowObj* window = *windowIt;

        if (!window->L->rect.contains(pos) || (window->flags & UI_WIDGET_FLAG_HIDDEN_BIT))
            continue;

        return get_widget_in_subtree((UIWidgetObj*)window, pos);
    }

    return nullptr;
}

void UIContextObj::pre_update()
{
    overlay.pre_update();

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

void UIContextObj::post_update()
{
    if (focusWidget && focusWidget == requestLooseFocus)
        focus_widget(nullptr);

    requestLooseFocus = nullptr;

    update_cursor_hint();
}

void UIContextObj::update_cursor_hint()
{
    cursorHint = CURSOR_TYPE_DEFAULT;

    // focused widget gets to hint first
    if (focusWidget)
    {
        cursorHint = widget_cursor_hint(focusWidget);
        if (cursorHint != CURSOR_TYPE_DEFAULT)
            return;
    }

    // bubble up along hover widgets
    for (UIWidgetObj* widget = hoverWidgetLeaf; widget; widget = widget->parent)
    {
        cursorHint = widget_cursor_hint(widget);
        if (cursorHint != CURSOR_TYPE_DEFAULT)
            return;
    }
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

    obj = heap_new<UILayerObj>(MEMORY_USAGE_UI, this);
    obj->name = std::string(name);
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

    if (removed == requestLooseFocus)
        requestLooseFocus = nullptr;

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

            widget_on_event(widget, event);

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

            widget_on_event(widget, event);

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
        widget_on_event(focusWidget, event);

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
        widget_on_event(focusWidget, event);

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

        widget_on_event(dragWidget, dragEvent);

        if (onEvent)
            onEvent(UIWidget(dragWidget), dragEvent, user);
    }

    UIWidgetObj* leaf = get_widget(cursorPos);
    hover_widget(leaf);

    UIEvent mousePosEvent{};
    mousePosEvent.type = UI_EVENT_MOUSE_POSITION;
    mousePosEvent.mouse.position = cursorPos;

    while (leaf)
    {
        widget_on_event(leaf, get_local_event(leaf, mousePosEvent));
        leaf = leaf->parent;
    }

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
        overlay.clear_windows();
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
    {
        overlay.clear_windows();
        return false;
    }

    widget = get_event_handler(widget, event);
    if (!widget)
    {
        overlay.clear_windows();
        return false;
    }

    if (overlay.find_widget(widget) < 0)
        overlay.clear_windows();

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
    if (!mObj->deferredLayerDestruction.empty())
        mObj->pre_update();

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
    obj->fontMonospace = info.fontMono;
    obj->user = info.user;
    obj->onEvent = info.onEvent;
    obj->theme = info.theme;

    constexpr size_t widgetsPerPage = 32;

    PoolAllocatorInfo paI{};
    paI.usage = MEMORY_USAGE_UI;
    paI.pageSize = widgetsPerPage;
    paI.isMultiPage = true;
    paI.blockSize = sizeof(UIWidgetObj);
    obj->widgetObjPA = PoolAllocator::create(paI);

    paI.blockSize = sizeof(UIWidgetUnion);
    obj->widgetUnionPA = PoolAllocator::create(paI);

    paI.blockSize = sizeof(UIWidgetLayout);
    obj->widgetLayoutPA = PoolAllocator::create(paI);

    obj->overlay.startup(obj);

    return UIContext(obj);
}

void UIContext::destroy(UIContext ctx)
{
    UIContextObj* obj = ctx.unwrap();

    for (UILayerObj* layer : obj->layers)
        obj->deferredLayerDestruction.insert(layer);

    obj->pre_update();
    LD_ASSERT(obj->layers.empty());

    obj->overlay.cleanup();

    PoolAllocator::destroy(obj->widgetLayoutPA);
    PoolAllocator::destroy(obj->widgetUnionPA);
    PoolAllocator::destroy(obj->widgetObjPA);
    heap_delete<UIContextObj>(obj);
}

void UIContext::update(Vec2 extent, float delta)
{
    LD_PROFILE_SCOPE;

    mObj->pre_update();

    if (mObj->overlay.workspace.get_root_rect().get_size() != extent)
        mObj->overlay.workspace.set_rect(Rect(0.0f, 0.0f, extent.x, extent.y));
    mObj->overlay.layerObj->update(delta);
    mObj->overlay.layerObj->layout();

    for (UILayerObj* layer : mObj->layers)
    {
        layer->update(delta);
        layer->layout();
    }

    mObj->post_update();
}

void UIContext::render(ScreenRenderComponent renderer)
{
    LD_PROFILE_SCOPE;

    // NOTE: Currently it is possible for the user to destroy UI objects
    //       between update() and render(), and this is required to avoid
    //       rendering out-of-date objects waiting for removal.
    mObj->pre_update();

    for (UILayerObj* layer : mObj->layers)
    {
        UILayer(layer).render(renderer);
    }

    // always drawn after user created layers
    UILayer(mObj->overlay.layerObj).render(renderer);
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

void UIContext::clear_overlay_windows()
{
    mObj->overlay.clear_windows();
}

int UIContext::get_overlay_level()
{
    return mObj->overlay.get_level();
}

UIWindow UIContext::set_overlay_level(int level)
{
    return mObj->overlay.set_level(level);
}

CursorType UIContext::get_cursor_hint()
{
    return mObj->cursorHint;
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