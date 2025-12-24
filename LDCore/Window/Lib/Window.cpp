#include <Ludens/Event/Event.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Log/Log.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/System/Memory.h>
#include <Ludens/Window/Window.h>

#include <GLFW/glfw3.h>

#include "InputInternal.h"
#include "WindowObj.h"

namespace LD {

static Log sLog("Window");
static WindowObj* sMainWindowObj = nullptr;

// regression test against GLFW version
static_assert(CURSOR_TYPE_DEFAULT + GLFW_ARROW_CURSOR == GLFW_ARROW_CURSOR);
static_assert(CURSOR_TYPE_IBEAM + GLFW_ARROW_CURSOR == GLFW_IBEAM_CURSOR);
static_assert(CURSOR_TYPE_CROSSHAIR + GLFW_ARROW_CURSOR == GLFW_CROSSHAIR_CURSOR);
static_assert(CURSOR_TYPE_HAND + GLFW_ARROW_CURSOR == GLFW_HAND_CURSOR);
static_assert(CURSOR_TYPE_HRESIZE + GLFW_ARROW_CURSOR == GLFW_HRESIZE_CURSOR);
static_assert(CURSOR_TYPE_VRESIZE + GLFW_ARROW_CURSOR == GLFW_VRESIZE_CURSOR);

WindowObj::WindowObj(const WindowInfo& windowI)
    : mHandle(nullptr), mWidth(windowI.width), mHeight(windowI.height), mUser(windowI.user), mOnEvent(windowI.onEvent)
{
    LD_PROFILE_SCOPE;

    for (int i = 0; i < (int)CURSOR_TYPE_ENUM_COUNT; i++)
        mCursors[i] = nullptr;

    int result = glfwInit();
    LD_ASSERT(result == GLFW_TRUE);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    mHandle = glfwCreateWindow((int)mWidth, (int)mHeight, windowI.name, nullptr, nullptr);
    glfwSetWindowUserPointer(mHandle, this);
    glfwSetWindowSizeCallback(mHandle, &WindowObj::size_callback);
    glfwSetKeyCallback(mHandle, &WindowObj::key_callback);
    glfwSetMouseButtonCallback(mHandle, &WindowObj::mouse_button_callback);
    glfwSetCursorPosCallback(mHandle, &WindowObj::cursor_pos_callback);
    glfwSetScrollCallback(mHandle, &WindowObj::scroll_callback);

    if (windowI.hintBorderColor != 0)
        hint_border_color(windowI.hintBorderColor);

    if (windowI.hintTitleBarColor != 0)
        hint_title_bar_color(windowI.hintTitleBarColor);

    if (windowI.hintTitleBarTextColor != 0)
        hint_title_bar_text_color(windowI.hintTitleBarTextColor);
}

WindowObj::~WindowObj()
{
    LD_PROFILE_SCOPE;

    for (int i = 0; i < (int)CURSOR_TYPE_ENUM_COUNT; i++)
        glfwDestroyCursor(mCursors[i]);

    glfwDestroyWindow(mHandle);
    glfwTerminate();
}

void WindowObj::get_cursor_pos(double& xpos, double& ypos)
{
    glfwGetCursorPos(mHandle, &xpos, &ypos);
}

void WindowObj::size_callback(GLFWwindow* window, int width, int height)
{
    WindowObj* obj = (WindowObj*)glfwGetWindowUserPointer(window);

    obj->mWidth = width;
    obj->mHeight = height;

    WindowResizeEvent event(width, height);
    Window::on_event(&event);
}

void WindowObj::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        bool firstPress = action == GLFW_PRESS;

        if (firstPress)
            Input::sKeyState[key] |= (PRESSED_BIT | PRESSED_THIS_FRAME_BIT);

        KeyDownEvent event((KeyCode)key, !firstPress);
        Window::on_event(&event);
    }
    else if (action == GLFW_RELEASE)
    {
        Input::sKeyState[key] = RELEASED_THIS_FRAME_BIT;

        KeyUpEvent event((KeyCode)key);
        Window::on_event(&event);
    }
}

void WindowObj::mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (action == GLFW_REPEAT)
        return;

    if (action == GLFW_PRESS)
    {
        Input::sMouseState[button] |= (PRESSED_BIT | PRESSED_THIS_FRAME_BIT);

        MouseDownEvent event((MouseButton)button);
        Window::on_event(&event);
    }
    else if (action == GLFW_RELEASE)
    {
        Input::sMouseState[button] = RELEASED_THIS_FRAME_BIT;

        MouseUpEvent event((MouseButton)button);
        Window::on_event(&event);
    }
}

void WindowObj::cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
    MouseMotionEvent event((float)xpos, (float)ypos);
    Window::on_event(&event);
}

void WindowObj::scroll_callback(GLFWwindow* handle, double xoffset, double yoffset)
{
    ScrollEvent event((float)xoffset, (float)yoffset);
    Window::on_event(&event);
}

bool WindowObj::is_open()
{
    return !glfwWindowShouldClose(mHandle);
}

double WindowObj::get_time()
{
    return glfwGetTime();
}

void WindowObj::frame_boundary()
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
}

void WindowObj::poll_events()
{
    LD_PROFILE_SCOPE;

    glfwPollEvents();
}

void WindowObj::on_event(const Event* event)
{
    LD_PROFILE_SCOPE;

    if (!mOnEvent)
        return;

    mOnEvent(event, mUser);
}

void WindowObj::set_cursor_mode_normal()
{
    glfwSetInputMode(mHandle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void WindowObj::set_cursor_mode_disabled()
{
    glfwSetInputMode(mHandle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void WindowObj::hint_title_bar_text(const char* cstr)
{
    glfwSetWindowTitle(mHandle, cstr);
}

void WindowObj::hint_cursor_shape(CursorType cursor)
{
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

    glfwSetCursor(mHandle, mCursors[cursorIdx]);
}

//
// Public API
//

Window Window::create(const WindowInfo& windowI)
{
    LD_ASSERT(!sMainWindowObj); // singleton

    sMainWindowObj = heap_new<WindowObj>(MEMORY_USAGE_MISC, windowI);

    return Window(sMainWindowObj);
}

void Window::destroy(Window window)
{
    LD_ASSERT(sMainWindowObj == window.unwrap()); // singleton

    heap_delete<WindowObj>(sMainWindowObj);
    sMainWindowObj = nullptr;
}

uint32_t Window::width() const
{
    return mObj->width();
}

uint32_t Window::height() const
{
    return mObj->height();
}

Vec2 Window::extent() const
{
    return Vec2((float)width(), (float)height());
}

float Window::aspect_ratio() const
{
    return mObj->aspect_ratio();
}

bool Window::is_minimized()
{
    return mObj->width() == 0 || mObj->height() == 0;
}

bool Window::is_open()
{
    return mObj->is_open();
}

void Window::poll_events()
{
    LD_PROFILE_SCOPE;

    // updates Window delta time
    mObj->frame_boundary();

    // updates input state for polling
    Input::frame_boundary();

    double xpos, ypos;
    mObj->get_cursor_pos(xpos, ypos);

    static bool sIsFirstFrame = true;

    if (sIsFirstFrame)
    {
        sIsFirstFrame = false;
        Input::sMouseCursorX = (float)xpos;
        Input::sMouseCursorY = (float)ypos;
    }

    Input::sMouseCursorDeltaX = xpos - Input::sMouseCursorX;
    Input::sMouseCursorDeltaY = ypos - Input::sMouseCursorY;
    Input::sMouseCursorX = (float)xpos;
    Input::sMouseCursorY = (float)ypos;

    mObj->poll_events();
}

GLFWwindow* Window::get_glfw_window()
{
    return mObj->get_glfw_handle();
}

Window Window::get()
{
    return Window(sMainWindowObj);
}

void Window::on_event(const Event* event)
{
    if (!sMainWindowObj)
        return;

    sMainWindowObj->on_event(event);
}

double Window::get_time()
{
    return mObj->get_time();
}

double Window::get_delta_time()
{
    return mObj->get_delta_time();
}

void Window::exit()
{
    mObj->exit();
}

void Window::set_cursor_mode_normal()
{
    mObj->set_cursor_mode_normal();

    double xpos, ypos;
    mObj->get_cursor_pos(xpos, ypos);

    Input::sMouseCursorDeltaX = 0.0f;
    Input::sMouseCursorDeltaY = 0.0f;
    Input::sMouseCursorX = (float)xpos;
    Input::sMouseCursorY = (float)ypos;
}

void Window::set_cursor_mode_disabled()
{
    mObj->set_cursor_mode_disabled();
}

void Window::hint_border_color(Color color)
{
    mObj->hint_border_color(color);
}

void Window::hint_title_bar_color(Color color)
{
    mObj->hint_title_bar_color(color);
}

void Window::hint_title_bar_text_color(Color color)
{
    mObj->hint_title_bar_text_color(color);
}

void Window::hint_title_bar_text(const char* cstr)
{
    if (!cstr)
        return;

    mObj->hint_title_bar_text(cstr);
}

void Window::hint_cursor_shape(CursorType cursor)
{
    mObj->hint_cursor_shape(cursor);
}

} // namespace LD