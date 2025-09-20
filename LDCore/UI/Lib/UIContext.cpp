#include "UIObj.h"
#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Bitwise.h>
#include <Ludens/Header/Math/Vec2.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/System/Memory.h>
#include <Ludens/UI/UIContext.h>
#include <algorithm>
#include <cstdint>
#include <vector>

namespace LD {

enum UIWidgetFilterBit
{
    WIDGET_FILTER_KEY_BIT = LD_BIT(0),
    WIDGET_FILTER_MOUSE_BIT = LD_BIT(1),
    WIDGET_FILTER_HOVER_BIT = LD_BIT(2),
    WIDGET_FILTER_DRAG_BIT = LD_BIT(3),
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
    obj->theme = window->ctx->theme;
    obj->scrollOffset = Vec2(0.0f);
    obj->flags = 0;

    window->widgets.push_back(obj);
    parent->append_child(obj);

    return obj;
}

void UIContextObj::free_widget(UIWidgetObj* widget)
{
    UIWidgetObj* parent = widget->parent;
    if (parent)
        parent->remove_child(widget);

    UIWindowObj* window = widget->window;
    size_t count = std::erase(window->widgets, widget);
    LD_ASSERT(count == 1);

    if (widget == dragWidget)
        dragWidget = nullptr;

    if (widget == pressWidget)
        pressWidget = nullptr;

    if (widget == cursorWidget)
        cursorWidget = nullptr;

    ui_obj_cleanup(widget);
    widgetPA.free(widget);
}

UIWidgetObj* UIContextObj::get_widget(const Vec2& pos, int filter)
{
    for (auto ite = windows.rbegin(); ite != windows.rend(); ite++)
    {
        UIWindowObj* window = *ite;

        if (!window->layout.rect.contains(pos) || (window->flags & UI_WIDGET_FLAG_HIDDEN_BIT))
            continue;

        if (window->flags & UI_WIDGET_FLAG_BLOCK_INPUT_BIT)
            return nullptr;

        return get_widget_at_pos((UIWidgetObj*)window, pos, filter);
    }

    return nullptr;
}

void UIContextObj::pre_update(float delta)
{
    for (UIWindowObj* window : deferredWindowDestruction)
    {
        heap_delete<UIWindowObj>(window);

        size_t count = std::erase(windows, window);
        LD_ASSERT(count == 1);
    }

    deferredWindowDestruction.clear();
}

void UIContextObj::raise_window(UIWindowObj* window)
{
    auto ite = std::find(windows.begin(), windows.end(), window);

    if (ite == windows.end())
        return;

    windows.erase(ite);
    windows.push_back(window);
}

void UIContext::input_mouse_position(const Vec2& pos)
{
    LD_PROFILE_SCOPE;

    mObj->cursorPos = pos;

    if (mObj->dragWidget)
    {
        UIWidgetObj* de = mObj->dragWidget;
        de->cb.onDrag({de}, mObj->dragMouseButton, mObj->cursorPos, false);
    }

    UIWidgetObj* prev = mObj->cursorWidget;
    UIWidgetObj* next = mObj->get_widget(pos, WIDGET_FILTER_HOVER_BIT);

    if (next)
    {
        if (next != prev && prev && prev->cb.onHover)
            prev->cb.onHover({prev}, UI_MOUSE_LEAVE);

        if (next != prev && next->cb.onHover)
            next->cb.onHover({next}, UI_MOUSE_ENTER);

        mObj->cursorWidget = next;
        return;
    }

    if (prev && prev->cb.onHover)
        prev->cb.onHover({prev}, UI_MOUSE_LEAVE);

    mObj->cursorWidget = nullptr;
}

void UIContext::input_mouse_down(MouseButton btn)
{
    UIWidgetObj* widget = mObj->get_widget(mObj->cursorPos, WIDGET_FILTER_MOUSE_BIT | WIDGET_FILTER_DRAG_BIT);

    if (!widget)
        return;

    bool blockInput = (widget->flags & UI_WIDGET_FLAG_BLOCK_INPUT_BIT);

    if (!blockInput && widget->cb.onDrag)
    {
        mObj->dragStartPos = mObj->cursorPos;
        mObj->dragWidget = widget;
        mObj->dragMouseButton = btn;

        widget->cb.onDrag({widget}, btn, mObj->cursorPos, true);
    }

    if (!blockInput && widget->cb.onMouse)
    {
        Vec2 localPos = mObj->cursorPos - widget->layout.rect.get_pos();
        widget->cb.onMouse({widget}, localPos, btn, UI_MOUSE_DOWN);
        mObj->pressWidget = widget;
    }
}

void UIContext::input_mouse_up(MouseButton btn)
{
    mObj->dragWidget = nullptr;
    mObj->pressWidget = nullptr;

    UIWidgetObj* widget = mObj->get_widget(mObj->cursorPos, WIDGET_FILTER_MOUSE_BIT);

    if (!widget)
        return;

    bool blockInput = (widget->flags & UI_WIDGET_FLAG_BLOCK_INPUT_BIT);

    if (!blockInput && widget->cb.onMouse)
    {
        Vec2 localPos = mObj->cursorPos - widget->layout.rect.get_pos();
        widget->cb.onMouse({widget}, localPos, btn, UI_MOUSE_UP);
    }
}

void UIContext::input_key_down(KeyCode key)
{
    UIWidgetObj* widget = mObj->get_widget(mObj->cursorPos, WIDGET_FILTER_KEY_BIT);

    if (!widget)
        return;

    bool blockInput = (widget->flags & UI_WIDGET_FLAG_BLOCK_INPUT_BIT);

    if (!blockInput && widget->cb.onKey)
        widget->cb.onKey({widget}, key, UI_KEY_DOWN);
}

void UIContext::input_key_up(KeyCode key)
{
    UIWidgetObj* widget = mObj->get_widget(mObj->cursorPos, WIDGET_FILTER_KEY_BIT);

    if (!widget)
        return;

    bool blockInput = (widget->flags & UI_WIDGET_FLAG_BLOCK_INPUT_BIT);

    if (!blockInput && widget->cb.onKey)
        widget->cb.onKey({widget}, key, UI_KEY_UP);
}

void UIContext::layout()
{
    LD_PROFILE_SCOPE;

    // TODO: window flags to skip layout pass
    for (UIWindowObj* window : mObj->windows)
    {
        ui_layout(window);
    }
}

UIWindow UIContext::add_window(const UILayoutInfo& layoutI, const UIWindowInfo& windowI, void* user)
{
    UIWindowObj* windowObj = heap_new<UIWindowObj>(MEMORY_USAGE_UI);
    windowObj->layout.info = layoutI;
    windowObj->user = user;
    windowObj->ctx = mObj;
    windowObj->type = UI_WIDGET_WINDOW;
    windowObj->window = windowObj;
    windowObj->node = {windowObj};
    windowObj->flags = 0;
    windowObj->theme = mObj->theme;

    if (windowI.name)
        windowObj->name = std::string(windowI.name);

    if (windowI.hidden)
        windowObj->flags |= UI_WIDGET_FLAG_HIDDEN_BIT;

    if (windowI.drawWithScissor)
        windowObj->flags |= UI_WIDGET_FLAG_DRAW_WITH_SCISSOR_BIT;

    if (windowI.defaultMouseControls)
        windowObj->cb.onDrag = UIWindowObj::on_drag;

    mObj->windows.push_back(windowObj);

    return {(UIWidgetObj*)windowObj};
}

void UIContext::remove_window(UIWindow window)
{
    UIWindowObj* obj = (UIWindowObj*)window.unwrap();

    // window destruction is deferred so we don't modify
    // window hierarchy while iterating it.
    mObj->deferredWindowDestruction.insert(obj);
}

void UIContext::get_windows(std::vector<UIWindow>& windows)
{
    windows.resize(mObj->windows.size());

    for (size_t i = 0; i < mObj->windows.size(); i++)
        windows[i] = {mObj->windows[i]};
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

    return {obj};
}

void UIContext::destroy(UIContext ctx)
{
    UIContextObj* obj = ctx.unwrap();

    for (UIWindowObj* window : obj->windows)
        heap_delete<UIWindowObj>(window);
    obj->windows.clear();

    PoolAllocator::destroy(obj->widgetPA);
    heap_delete<UIContextObj>(obj);
}

void UIContext::update(float delta)
{
    mObj->pre_update(delta);

    layout();

    for (UIWindowObj* window : mObj->windows)
    {
        if (window->cb.onUpdate)
            window->cb.onUpdate({window}, delta);

        // updates all widgets within window
        window->update(delta);
    }
}

} // namespace LD