#include "UIFWidgetObj.h"
#include <Ludens/Application/Input.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/System/Memory.h>
#include <Ludens/UIF/UIFTheme.h>
#include <Ludens/UIF/UIFWindow.h>
#include <vector>

namespace LD {
namespace UIF {

void WindowObj::update(float delta)
{
    for (WidgetObj* widget : children)
    {
        switch (widget->type)
        {
        case WIDGET_TYPE_TOGGLE:
            widget->as.toggle.anim.update(delta);
            break;
        }
    }
}

void WindowObj::on_drag(void* user, UIElement e, MouseButton btn, const Vec2& dragPos, bool begin)
{
    WindowObj* obj = (WindowObj*)user;
    Rect rect = obj->handle.get_rect();

    if (begin)
    {
        obj->dragResize = btn == MOUSE_BUTTON_RIGHT; // right button to resize, left button to reposition
        obj->dragOffset = dragPos - rect.get_pos();  // fixed drag offset
        obj->dragBeginPos = dragPos;
        obj->dragBeginSize = rect.get_size();
    }

    if (obj->dragResize)
    {
        Vec2 delta = dragPos - obj->dragBeginPos;
        obj->handle.set_size(obj->dragBeginSize + delta);
    }
    else
        obj->handle.set_pos(dragPos - obj->dragOffset);
}

Context Context::create(const ContextInfo& info)
{
    ContextObj* obj = heap_new<ContextObj>(MEMORY_USAGE_UI);
    obj->handle = UIContext::create();
    obj->fontAtlas = info.fontAtlas;
    obj->fontAtlasImage = info.fontAtlasImage;

    LD_ASSERT(obj->fontAtlas && obj->fontAtlasImage);

    get_default_theme(obj->theme);

    return {obj};
}

void Context::destroy(Context ctx)
{
    ContextObj* obj = ctx;
    UIContext::destroy(obj->handle);

    heap_delete<ContextObj>(obj);
}

void Context::update(float dt)
{
    float x, y;
    UIContext ctx = mObj->handle;

    if (Input::get_mouse_motion(x, y))
    {
        Input::get_mouse_position(x, y);
        ctx.input_mouse_position({x, y});
    }
    for (MouseButton btn = MOUSE_BUTTON_LEFT; btn < MOUSE_BUTTON_ENUM_LAST; btn = (MouseButton)(btn + 1))
    {
        if (Input::get_mouse_down(btn))
            ctx.input_mouse_press(btn);
        if (Input::get_mouse_up(btn))
            ctx.input_mouse_release(btn);
    }

    ctx.layout();

    for (WidgetObj* window : mObj->windows)
    {
        window->as.window.update(dt);
    }
}

Window Context::add_window(const UILayoutInfo& layoutI, const WindowInfo& windowI)
{
    UIWindowInfo nativeWindowI{};
    nativeWindowI.name = windowI.name;

    WidgetObj* obj = (WidgetObj*)heap_malloc(sizeof(WidgetObj), MEMORY_USAGE_UI);
    WindowObj* windowObj = &obj->as.window;

    new (windowObj) WindowObj();
    windowObj->name = windowI.name;
    windowObj->ctx = mObj;

    UIWindow nativeHandle = mObj->handle.add_window(layoutI, nativeWindowI, windowObj);
    obj->type = WIDGET_TYPE_WINDOW;
    obj->node = {obj};
    obj->user = nullptr;
    obj->window = &obj->as.window;
    obj->handle = nativeHandle;
    obj->as.window.handle = nativeHandle;
    obj->as.window.handle.set_on_drag(&WindowObj::on_drag);

    mObj->windows.push_back(obj);

    return {obj};
}

void Context::get_windows(std::vector<Window>& windows)
{
    windows.resize(mObj->windows.size());

    for (size_t i = 0; i < windows.size(); i++)
        windows[i] = {mObj->windows[i]};
}

WidgetNode& Window::node()
{
    return mObj->node;
}

void Window::set_pos(const Vec2& pos)
{
    mObj->as.window.handle.set_pos(pos);
}

void Window::set_size(const Vec2& size)
{
    mObj->as.window.handle.set_size(size);
}

void Window::get_children(std::vector<Widget>& children)
{
    children = mObj->as.window.children;
}

Rect Window::get_rect() const
{
    return mObj->handle.get_rect();
}

std::string Window::get_name() const
{
    return mObj->as.window.name;
}

void Window::show()
{
    UIWindow handle = mObj->window->handle;

    handle.show();
}

void Window::hide()
{
    UIWindow handle = mObj->window->handle;

    handle.hide();
}

bool Window::is_hidden()
{
    UIWindow handle = mObj->window->handle;

    return handle.is_hidden();
}

} // namespace UIF
} // namespace LD