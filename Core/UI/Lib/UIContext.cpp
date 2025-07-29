#include "UIObj.h"
#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Bitwise.h>
#include <Ludens/Header/Math/Vec2.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/System/Memory.h>
#include <Ludens/UI/UIContext.h>
#include <cstdint>
#include <vector>

namespace LD {

/// @brief get the element at position
/// @param root the root element to search recursively
/// @param pos global position
/// @return the element at position, or null if position is out of bounds
static UIWidgetObj* get_widget_at_pos(UIWidgetObj* root, const Vec2& pos)
{
    if (!root->layout.rect.contains(pos))
        return nullptr;

    for (UIWidgetObj* child = root->child; child; child = child->next)
    {
        if (!child->layout.rect.contains(pos))
            continue;

        UIWidgetObj* result = get_widget_at_pos(child, pos);

        if (result)
            return result;
    }

    return root;
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

    window->widgets.push_back(obj);
    parent->append_child(obj);

    return obj;
}

void UIContext::input_mouse_position(const Vec2& pos)
{
    LD_PROFILE_SCOPE;

    mObj->cursorPos = pos;

    if (mObj->dragElement)
    {
        UIWidgetObj* de = mObj->dragElement;
        de->cb.onDrag({de}, mObj->dragMouseButton, mObj->cursorPos, false);
    }

    UIWidgetObj* prev = mObj->cursorElement;
    UIWidgetObj* next = nullptr;

    // last drawn window takes input first
    for (auto ite = mObj->windows.rbegin(); ite != mObj->windows.rend(); ite++)
    {
        UIWindowObj* window = *ite;

        if (window->isHidden || !window->layout.rect.contains(pos))
            continue;

        next = get_widget_at_pos((UIWidgetObj*)window, pos);

        if (next)
        {
            if (next != prev && prev && prev->cb.onLeave)
                prev->cb.onLeave({prev});

            if (next != prev && next->cb.onEnter)
                next->cb.onEnter({next});

            mObj->cursorElement = next;
            return;
        }
    }

    if (prev && prev->cb.onLeave)
        prev->cb.onLeave({prev});

    mObj->cursorElement = nullptr;
}

void UIContext::input_mouse_down(MouseButton btn)
{
    UIWidgetObj* widget = mObj->cursorElement;
    if (!widget)
        return;

    if (widget->cb.onDrag)
    {
        mObj->dragStartPos = mObj->cursorPos;
        mObj->dragElement = widget;
        mObj->dragMouseButton = btn;

        widget->cb.onDrag({widget}, btn, mObj->cursorPos, true);
    }

    if (widget->cb.onMouseDown)
    {
        Vec2 localPos = mObj->cursorPos - widget->layout.rect.get_pos();
        widget->cb.onMouseDown({widget}, localPos, btn);
        mObj->pressElement = widget;
    }
}

void UIContext::input_mouse_up(MouseButton btn)
{
    mObj->dragElement = nullptr;
    mObj->pressElement = nullptr;

    UIWidgetObj* widget = mObj->cursorElement;
    if (!widget)
        return;

    if (widget->cb.onMouseUp)
    {
        Vec2 localPos = mObj->cursorPos - widget->layout.rect.get_pos();
        widget->cb.onMouseUp({widget}, localPos, btn);
    }
}

void UIContext::input_key_down(KeyCode key)
{
    UIWidgetObj* widget = mObj->cursorElement;
    if (!widget)
        return;

    if (widget->cb.onKeyDown)
        widget->cb.onKeyDown({widget}, key);
}

void UIContext::input_key_up(KeyCode key)
{
    UIWidgetObj* widget = mObj->cursorElement;
    if (!widget)
        return;

    if (widget->cb.onKeyUp)
        widget->cb.onKeyUp({widget}, key);
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
    windowObj->isHidden = false;

    if (windowI.defaultMouseControls)
        windowObj->cb.onDrag = UIWindowObj::on_drag;

    mObj->windows.push_back(windowObj);

    return {(UIWidgetObj*)windowObj};
}

void UIContext::get_windows(std::vector<UIWindow>& windows)
{
    windows.resize(mObj->windows.size());

    for (size_t i = 0; i < mObj->windows.size(); i++)
        windows[i] = {mObj->windows[i]};
}

UIContext UIContext::create(const UIContextInfo& info)
{
    UIContextObj* obj = heap_new<UIContextObj>(MEMORY_USAGE_UI);
    obj->fontAtlas = info.fontAtlas;
    obj->fontAtlasImage = info.fontAtlasImage;

    PoolAllocatorInfo paI{};
    paI.blockSize = sizeof(UIWidgetObj);
    paI.isMultiPage = true;
    paI.pageSize = 64; // widgets per memory page
    paI.usage = MEMORY_USAGE_UI;
    obj->widgetPA = PoolAllocator::create(paI);

    //LD_ASSERT(obj->fontAtlas && obj->fontAtlasImage);

    get_default_theme(obj->theme);

    return {obj};
}

void UIContext::destroy(UIContext ctx)
{
    UIContextObj* obj = ctx;
    PoolAllocator::destroy(obj->widgetPA);

    for (UIWindowObj* window : obj->windows)
        heap_delete<UIWindowObj>(window);
    obj->windows.clear();

    heap_delete<UIContextObj>(obj);
}

void UIContext::update(float delta)
{
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