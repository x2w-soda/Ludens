#include <Ludens/DSA/HashMap.h>
#include <Ludens/DSA/IDCounter.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/Event/Event.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Log/Log.h>
#include <Ludens/Media/Bitmap.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/WindowRegistry/WindowRegistry.h>

#include "./Window.h"

#include <GLFW/glfw3.h>

namespace LD {

// regression test against GLFW version
static_assert(CURSOR_TYPE_DEFAULT + GLFW_ARROW_CURSOR == GLFW_ARROW_CURSOR);
static_assert(CURSOR_TYPE_IBEAM + GLFW_ARROW_CURSOR == GLFW_IBEAM_CURSOR);
static_assert(CURSOR_TYPE_CROSSHAIR + GLFW_ARROW_CURSOR == GLFW_CROSSHAIR_CURSOR);
static_assert(CURSOR_TYPE_HAND + GLFW_ARROW_CURSOR == GLFW_HAND_CURSOR);
static_assert(CURSOR_TYPE_HRESIZE + GLFW_ARROW_CURSOR == GLFW_HRESIZE_CURSOR);
static_assert(CURSOR_TYPE_VRESIZE + GLFW_ARROW_CURSOR == GLFW_VRESIZE_CURSOR);

static WindowRegistryObj* sWindowRegistry = nullptr;
static Log sLog("WindowRegistry");

WindowRegistryObj::WindowRegistryObj()
{
    LD_PROFILE_SCOPE;

    for (int i = 0; i < (int)CURSOR_TYPE_ENUM_COUNT; i++)
        mCursors[i] = nullptr;

    int result = glfwInit();
    if (result != GLFW_TRUE)
    {
        sLog.error("glfwInit failed");
        LD_UNREACHABLE;
        return;
    }
}

WindowRegistryObj::~WindowRegistryObj()
{
    LD_PROFILE_SCOPE;
    LD_ASSERT(mWindows.empty());

    for (int i = 0; i < (int)CURSOR_TYPE_ENUM_COUNT; i++)
    {
        glfwDestroyCursor(mCursors[i]);
        mCursors[i] = nullptr;
    }

    glfwTerminate();
}

WindowObj* WindowRegistryObj::create_window(const WindowInfo& windowI, WindowID parentID)
{
    LD_PROFILE_SCOPE;

    WindowID id = mIDCounter.get_id();

    if (parentID == 0)
    {
        LD_ASSERT(mWindows.empty()); // only root window has parentID of zero
        mRootID = id;
    }

    WindowObj* parentObj = parentID == 0 ? nullptr : mWindows[parentID];
    WindowObj* obj = heap_new<WindowObj>(MEMORY_USAGE_MISC, windowI, this, id, parentObj);

    mWindows[id] = obj;

    WindowCreateEvent event(id);
    notify_observers(&event);

    return obj;
}

void WindowRegistryObj::destroy_window(WindowID id)
{
    LD_PROFILE_SCOPE;

    if (!mWindows.contains(id))
        return;

    WindowID parentID = mWindows[id]->get_parent_id();

    destroy_window_subtree(id);

    if (parentID != 0)
    {
        LD_ASSERT(mWindows.contains(parentID));
        mWindows[parentID]->erase_child_id(id);
    }
}

void WindowRegistryObj::destroy_window_subtree(WindowID id)
{
    LD_ASSERT(mWindows.contains(id));

    WindowObj* obj = mWindows[id];
    const Vector<WindowID>& childrenID = obj->get_children_id();

    for (WindowID childID : childrenID)
    {
        destroy_window_subtree(childID);
    }

    WindowDestroyEvent event(id);
    notify_observers(&event);

    heap_delete<WindowObj>(obj);
    mWindows.erase(id);
}

void WindowRegistryObj::frame_boundary()
{
    static bool sIsFirstFrame = true;

    if (sIsFirstFrame)
    {
        sIsFirstFrame = false;
        mTimePrevFrame = glfwGetTime();
    }

    mTimeThisFrame = glfwGetTime();
    mTimeDelta = mTimeThisFrame - mTimePrevFrame;
    mTimePrevFrame = mTimeThisFrame;

    Vector<WindowID> toDestroy;

    for (auto it : mWindows)
    {
        WindowObj* obj = it.second;

        if (!obj->is_alive())
            toDestroy.push_back(it.first);

        obj->frame_boundary();
    }

    for (WindowID id : toDestroy)
        destroy_window(id);
}

void WindowRegistryObj::hint_window_cursor_shape(WindowID id, CursorType cursor)
{
    if (!mWindows.contains(id))
        return;

    GLFWwindow* window = mWindows[id]->get_glfw_handle();
    int cursorIdx = (int)cursor;

    if (!mCursors[cursorIdx])
    {
        mCursors[cursorIdx] = glfwCreateStandardCursor(cursorIdx + GLFW_ARROW_CURSOR);

        if (!mCursors[cursorIdx])
        {
            sLog.warn("glfwCreateStandardCursor failed for {}", cursorIdx);
            return;
        }
    }

    glfwSetCursor(window, mCursors[cursorIdx]);
}

void WindowRegistryObj::add_observer(WindowEventFn fn, void* user)
{
    LD_ASSERT(fn); // nullptr callback

    mObservers.add_observer(fn, user);
}

void WindowRegistryObj::remove_observer(WindowEventFn fn, void* user)
{
    mObservers.remove_observer(fn, user);
}

void WindowRegistryObj::notify_observers(const WindowEvent* event)
{
    mObservers.notify(event);
}

//
// Public API
//

WindowRegistry WindowRegistry::create(const WindowInfo& rootWindowInfo)
{
    LD_ASSERT(!sWindowRegistry); // singleton

    sWindowRegistry = heap_new<WindowRegistryObj>(MEMORY_USAGE_MISC);

    sWindowRegistry->create_window(rootWindowInfo, 0);

    return WindowRegistry(sWindowRegistry);
}

void WindowRegistry::destroy()
{
    LD_ASSERT(sWindowRegistry); // singleton

    WindowID rootID = sWindowRegistry->get_root_id();
    sWindowRegistry->destroy_window(rootID);

    heap_delete<WindowRegistryObj>(sWindowRegistry);
    sWindowRegistry = nullptr;
}

WindowRegistry WindowRegistry::get()
{
    return WindowRegistry(sWindowRegistry);
}

WindowID WindowRegistry::get_root_id()
{
    return mObj->get_root_id();
}

double WindowRegistry::get_delta_time()
{
    return mObj->get_delta_time();
}

void WindowRegistry::poll_events()
{
    LD_PROFILE_SCOPE;

    // updates registry delta time
    // destroys stale windows
    // reset input polling
    mObj->frame_boundary();

    glfwPollEvents();
}

WindowID WindowRegistry::create_window(const WindowInfo& windowInfo, WindowID parentID)
{
    if (parentID == 0)
        return 0;

    WindowObj* obj = mObj->create_window(windowInfo, parentID);
    if (!obj)
        return 0;

    return obj->get_id();
}

void WindowRegistry::close_window(WindowID id)
{
    WindowObj* obj = mObj->get_window(id);
    if (!obj)
        return;

    obj->close();
}

void WindowRegistry::add_observer(WindowEventFn fn, void* user)
{
    mObj->add_observer(fn, user);
}

void WindowRegistry::remove_observer(WindowEventFn fn, void* user)
{
    mObj->remove_observer(fn, user);
}

GLFWwindow* WindowRegistry::get_window_glfw_handle(WindowID id)
{
    WindowObj* obj = mObj->get_window(id);
    if (!obj)
        return nullptr;

    return obj->get_glfw_handle();
}

bool WindowRegistry::is_window_open(WindowID id)
{
    WindowObj* obj = mObj->get_window(id);
    if (!obj)
        return false;

    return obj->is_alive() && !glfwWindowShouldClose(obj->get_glfw_handle());
}

bool WindowRegistry::is_window_minimized(WindowID id)
{
    WindowObj* obj = mObj->get_window(id);
    if (!obj)
        return false;

    return obj->width() == 0 || obj->height() == 0;
}

Vec2 WindowRegistry::get_window_extent(WindowID id)
{
    WindowObj* obj = mObj->get_window(id);
    if (!obj)
        return {};

    return obj->extent();
}

float WindowRegistry::get_window_aspect_ratio(WindowID id)
{
    WindowObj* obj = mObj->get_window(id);
    if (!obj)
        return 0.0f;

    return obj->aspect_ratio();
}

bool WindowRegistry::get_window_key(WindowID id, KeyCode key)
{
    WindowObj* obj = mObj->get_window(id);
    if (!obj)
        return false;

    return obj->get_key(key);
}

bool WindowRegistry::get_window_key_up(WindowID id, KeyCode key)
{
    WindowObj* obj = mObj->get_window(id);
    if (!obj)
        return false;

    return obj->get_key_up(key);
}

bool WindowRegistry::get_window_key_down(WindowID id, KeyCode key)
{
    WindowObj* obj = mObj->get_window(id);
    if (!obj)
        return false;

    return obj->get_key_down(key);
}

bool WindowRegistry::get_window_mouse(WindowID id, MouseButton button)
{
    WindowObj* obj = mObj->get_window(id);
    if (!obj)
        return false;

    return obj->get_mouse(button);
}

bool WindowRegistry::get_window_mouse_up(WindowID id, MouseButton button)
{
    WindowObj* obj = mObj->get_window(id);
    if (!obj)
        return false;

    return obj->get_mouse_up(button);
}

bool WindowRegistry::get_window_mouse_down(WindowID id, MouseButton button)
{
    WindowObj* obj = mObj->get_window(id);
    if (!obj)
        return false;

    return obj->get_mouse_down(button);
}

bool WindowRegistry::get_window_mouse_position(WindowID id, float& x, float& y)
{
    WindowObj* obj = mObj->get_window(id);
    if (!obj)
        return false;

    obj->get_mouse_position(x, y);
    return true;
}

bool WindowRegistry::get_window_mouse_motion(WindowID id, float& dx, float& dy)
{
    WindowObj* obj = mObj->get_window(id);
    if (!obj)
        return false;

    return obj->get_mouse_motion(dx, dy);
}

void WindowRegistry::hint_window_title_bar_text(WindowID id, const char* cstr)
{
    WindowObj* obj = mObj->get_window(id);
    if (!obj || !cstr)
        return;

    obj->hint_title_bar_text(cstr);
}

void WindowRegistry::hint_window_cursor_shape(WindowID id, CursorType cursor)
{
    mObj->hint_window_cursor_shape(id, cursor);
}

void WindowRegistry::set_window_cursor_mode_normal(WindowID id)
{
    WindowObj* obj = mObj->get_window(id);
    if (!obj)
        return;

    obj->set_cursor_mode_normal();
}

void WindowRegistry::set_window_cursor_mode_disabled(WindowID id)
{
    WindowObj* obj = mObj->get_window(id);
    if (!obj)
        return;

    obj->set_cursor_mode_disabled();
}

void WindowRegistry::hint_window_icon(WindowID id, int iconCount, Bitmap* icons)
{
    LD_PROFILE_SCOPE;

    WindowObj* obj = mObj->get_window(id);
    if (!obj || iconCount == 0 || !icons)
        return;

    obj->hint_icon(iconCount, icons);
}

void WindowRegistry::hint_window_border_color(WindowID id, Color color)
{
    WindowObj* obj = mObj->get_window(id);
    if (!obj)
        return;

    obj->hint_border_color(color);
}

void WindowRegistry::hint_window_title_bar_color(WindowID id, Color color)
{
    WindowObj* obj = mObj->get_window(id);
    if (!obj)
        return;

    obj->hint_title_bar_color(color);
}

void WindowRegistry::hint_window_title_bar_text_color(WindowID id, Color color)
{
    WindowObj* obj = mObj->get_window(id);
    if (!obj)
        return;

    obj->hint_title_bar_text_color(color);
}

} // namespace LD