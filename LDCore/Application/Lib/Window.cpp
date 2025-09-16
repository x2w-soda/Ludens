#include "Window.h"
#include "InputInternal.h"
#include <GLFW/glfw3.h>
#include <Ludens/Application/Application.h>
#include <Ludens/Application/Event.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Log/Log.h>

namespace LD {

static Log sLog("Application");

// regression test against GLFW version
static_assert(CURSOR_TYPE_DEFAULT + GLFW_ARROW_CURSOR == GLFW_ARROW_CURSOR);
static_assert(CURSOR_TYPE_IBEAM + GLFW_ARROW_CURSOR == GLFW_IBEAM_CURSOR);
static_assert(CURSOR_TYPE_CROSSHAIR + GLFW_ARROW_CURSOR == GLFW_CROSSHAIR_CURSOR);
static_assert(CURSOR_TYPE_HAND + GLFW_ARROW_CURSOR == GLFW_HAND_CURSOR);
static_assert(CURSOR_TYPE_HRESIZE + GLFW_ARROW_CURSOR == GLFW_HRESIZE_CURSOR);
static_assert(CURSOR_TYPE_VRESIZE + GLFW_ARROW_CURSOR == GLFW_VRESIZE_CURSOR);

Window::Window()
    : mHandle(nullptr), mWidth(0), mHeight(0)
{
    for (int i = 0; i < (int)CURSOR_TYPE_ENUM_COUNT; i++)
        mCursors[i] = nullptr;
}

void Window::startup(const ApplicationInfo& appI)
{
    LD_PROFILE_SCOPE;

    int result = glfwInit();
    LD_ASSERT(result == GLFW_TRUE);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    mHandle = glfwCreateWindow((int)appI.width, (int)appI.height, appI.name, nullptr, nullptr);
    mWidth = appI.width;
    mHeight = appI.height;
    glfwSetWindowUserPointer(mHandle, this);
    glfwSetWindowSizeCallback(mHandle, &Window::size_callback);
    glfwSetKeyCallback(mHandle, &Window::key_callback);
    glfwSetMouseButtonCallback(mHandle, &Window::mouse_button_callback);
    glfwSetCursorPosCallback(mHandle, &Window::cursor_pos_callback);

    if (appI.hintBorderColor != 0)
        hint_border_color(appI.hintBorderColor);

    if (appI.hintTitleBarColor != 0)
        hint_title_bar_color(appI.hintTitleBarColor);

    if (appI.hintTitleBarTextColor != 0)
        hint_title_bar_text_color(appI.hintTitleBarTextColor);
}

void Window::cleanup()
{
    LD_PROFILE_SCOPE;

    for (int i = 0; i < (int)CURSOR_TYPE_ENUM_COUNT; i++)
        glfwDestroyCursor(mCursors[i]);

    glfwDestroyWindow(mHandle);
    glfwTerminate();
}

GLFWwindow* Window::get_glfw_handle()
{
    return mHandle;
}

void Window::poll_events()
{
    LD_PROFILE_SCOPE;

    glfwPollEvents();
}

void Window::get_cursor_pos(double& xpos, double& ypos)
{
    glfwGetCursorPos(mHandle, &xpos, &ypos);
}

void Window::set_cursor_mode_normal()
{
    glfwSetInputMode(mHandle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void Window::set_cursor_mode_disabled()
{
    glfwSetInputMode(mHandle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

double Window::get_time()
{
    return glfwGetTime();
}

bool Window::is_open()
{
    return !glfwWindowShouldClose(mHandle);
}

void Window::size_callback(GLFWwindow* window, int width, int height)
{
    Window* w = (Window*)glfwGetWindowUserPointer(window);

    w->mWidth = width;
    w->mHeight = height;

    ApplicationResizeEvent event(width, height);
    Application::on_event(&event);
}

void Window::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        bool firstPress = action == GLFW_PRESS;

        if (firstPress)
            Input::sKeyState[key] |= (PRESSED_BIT | PRESSED_THIS_FRAME_BIT);

        KeyDownEvent event((KeyCode)key, !firstPress);
        Application::on_event(&event);
    }
    else if (action == GLFW_RELEASE)
    {
        Input::sKeyState[key] = RELEASED_THIS_FRAME_BIT;

        KeyUpEvent event((KeyCode)key);
        Application::on_event(&event);
    }
}

void Window::mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (action == GLFW_REPEAT)
        return;

    if (action == GLFW_PRESS)
    {
        Input::sMouseState[button] |= (PRESSED_BIT | PRESSED_THIS_FRAME_BIT);

        MouseDownEvent event((MouseButton)button);
        Application::on_event(&event);
    }
    else if (action == GLFW_RELEASE)
    {
        Input::sMouseState[button] = RELEASED_THIS_FRAME_BIT;

        MouseUpEvent event((MouseButton)button);
        Application::on_event(&event);
    }
}

void Window::cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
    MouseMotionEvent event((float)xpos, (float)ypos);
    Application::on_event(&event);
}

void Window::hint_title_bar_text(const char* cstr)
{
    glfwSetWindowTitle(mHandle, cstr);
}

void Window::hint_cursor_shape(CursorType cursor)
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

} // namespace LD