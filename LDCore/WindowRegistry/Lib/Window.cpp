#include <Ludens/DSA/Vector.h>
#include <Ludens/Event/Event.h>
#include <Ludens/Media/Bitmap.h>
#include <Ludens/Profiler/Profiler.h>

#include "./Window.h"

#include <GLFW/glfw3.h>

#define PRESSED_BIT 0x1
#define PRESSED_THIS_FRAME_BIT 0x2
#define RELEASED_THIS_FRAME_BIT 0x4

namespace LD {

// keycodes are defined to be identical to GLFW
static_assert(GLFW_KEY_LAST < KEY_CODE_ENUM_LAST);
static_assert(GLFW_MOUSE_BUTTON_LEFT == MOUSE_BUTTON_LEFT);
static_assert(GLFW_MOUSE_BUTTON_RIGHT == MOUSE_BUTTON_RIGHT);
static_assert(GLFW_MOUSE_BUTTON_MIDDLE == MOUSE_BUTTON_MIDDLE);

WindowObj::WindowObj(const WindowInfo& windowI, WindowRegistryObj* reg, WindowID ID, WindowID parentID)
    : mHandle(nullptr), mRegistry(reg), mID(ID), mParentID(parentID), mWidth(windowI.width), mHeight(windowI.height), mUser(windowI.user), mOnEvent(windowI.onEvent)
{
    LD_PROFILE_SCOPE;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // TODO: RDEVICE_BACKEND_OPENGL
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

    mMouseCursorX = 0.0f;
    mMouseCursorY = 0.0f;
    mMouseCursorDeltaX = 0.0f;
    mMouseCursorDeltaY = 0.0f;

    memset(mMouseState, 0, sizeof(mMouseState));
    memset(mKeyState, 0, sizeof(mKeyState));

    mIsAlive = true;
}

WindowObj::~WindowObj()
{
    LD_PROFILE_SCOPE;

    glfwDestroyWindow(mHandle);
}

void WindowObj::size_callback(GLFWwindow* window, int width, int height)
{
    WindowObj* obj = (WindowObj*)glfwGetWindowUserPointer(window);

    obj->mWidth = width;
    obj->mHeight = height;

    WindowResizeEvent event(obj->mID, width, height);
    obj->on_event(&event);
}

void WindowObj::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    auto* obj = (WindowObj*)glfwGetWindowUserPointer(window);

    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        bool firstPress = action == GLFW_PRESS;

        if (firstPress)
            obj->mKeyState[key] |= (PRESSED_BIT | PRESSED_THIS_FRAME_BIT);

        WindowKeyDownEvent event(obj->mID, (KeyCode)key, !firstPress);
        obj->on_event(&event);
    }
    else if (action == GLFW_RELEASE)
    {
        obj->mKeyState[key] = RELEASED_THIS_FRAME_BIT;

        WindowKeyUpEvent event(obj->mID, (KeyCode)key);
        obj->on_event(&event);
    }
}

void WindowObj::mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    auto* obj = (WindowObj*)glfwGetWindowUserPointer(window);

    if (action == GLFW_REPEAT)
        return;

    if (action == GLFW_PRESS)
    {
        obj->mMouseState[button] |= (PRESSED_BIT | PRESSED_THIS_FRAME_BIT);

        WindowMouseDownEvent event(obj->mID, (MouseButton)button);
        obj->on_event(&event);
    }
    else if (action == GLFW_RELEASE)
    {
        obj->mMouseState[button] = RELEASED_THIS_FRAME_BIT;

        WindowMouseUpEvent event(obj->mID, (MouseButton)button);
        obj->on_event(&event);
    }
}

void WindowObj::cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
    auto* obj = (WindowObj*)glfwGetWindowUserPointer(window);
    WindowMouseMotionEvent event(obj->mID, (float)xpos, (float)ypos);
    obj->on_event(&event);
}

void WindowObj::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    auto* obj = (WindowObj*)glfwGetWindowUserPointer(window);
    WindowScrollEvent event(obj->mID, (float)xoffset, (float)yoffset);
    obj->on_event(&event);
}

void WindowObj::on_event(const WindowEvent* event)
{
    LD_PROFILE_SCOPE;

    mRegistry->notify_observers(event);

    if (!mOnEvent)
        return;

    mOnEvent(event, mUser);
}

void WindowObj::set_cursor_mode_normal()
{
    glfwSetInputMode(mHandle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    double xpos, ypos;
    glfwGetCursorPos(mHandle, &xpos, &ypos);

    mMouseCursorDeltaX = 0.0f;
    mMouseCursorDeltaY = 0.0f;
    mMouseCursorX = (float)xpos;
    mMouseCursorY = (float)ypos;
}

void WindowObj::set_cursor_mode_disabled()
{
    glfwSetInputMode(mHandle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void WindowObj::hint_icon(int iconCount, Bitmap* icons)
{
    Vector<GLFWimage> images;

    for (int i = 0; i < iconCount; i++)
    {
        Bitmap icon = icons[i];

        if (!icon || icon.format() != BITMAP_FORMAT_RGBA8U)
            continue;

        images.emplace_back((int)icon.width(), (int)icon.height(), (unsigned char*)icon.data());
    }

    if (images.empty())
        return;

    glfwSetWindowIcon(mHandle, (int)images.size(), images.data());
}

void WindowObj::hint_title_bar_text(const char* cstr)
{
    glfwSetWindowTitle(mHandle, cstr);
}

void WindowObj::frame_boundary()
{
    if (!mIsAlive)
        return;

    for (int i = 0; i < KEY_CODE_ENUM_LAST; i++)
        mKeyState[i] &= ~(PRESSED_THIS_FRAME_BIT | RELEASED_THIS_FRAME_BIT);

    for (int i = 0; i < MOUSE_BUTTON_ENUM_LAST; i++)
        mMouseState[i] &= ~(PRESSED_THIS_FRAME_BIT | RELEASED_THIS_FRAME_BIT);

    double xpos, ypos;
    glfwGetCursorPos(mHandle, &xpos, &ypos);

    static bool sIsFirstFrame = true;

    if (sIsFirstFrame)
    {
        sIsFirstFrame = false;
        mMouseCursorX = (float)xpos;
        mMouseCursorY = (float)ypos;
    }

    mMouseCursorDeltaX = xpos - mMouseCursorX;
    mMouseCursorDeltaY = ypos - mMouseCursorY;
    mMouseCursorX = (float)xpos;
    mMouseCursorY = (float)ypos;

    mIsAlive = !glfwWindowShouldClose(mHandle);
}

bool WindowObj::get_key(KeyCode key)
{
    return mKeyState[key] & PRESSED_BIT;
}

bool WindowObj::get_key_down(KeyCode key)
{
    return mKeyState[key] & PRESSED_THIS_FRAME_BIT;
}

bool WindowObj::get_key_up(KeyCode key)
{
    return mKeyState[key] & RELEASED_THIS_FRAME_BIT;
}

bool WindowObj::get_mouse(MouseButton button)
{
    return mMouseState[button] & PRESSED_BIT;
}

bool WindowObj::get_mouse_down(MouseButton button)
{
    return mMouseState[button] & PRESSED_THIS_FRAME_BIT;
}

bool WindowObj::get_mouse_up(MouseButton button)
{
    return mMouseState[button] & RELEASED_THIS_FRAME_BIT;
}

void WindowObj::get_mouse_position(float& x, float& y)
{
    x = mMouseCursorX;
    y = mMouseCursorY;
}

bool WindowObj::get_mouse_motion(float& dx, float& dy)
{
    dx = mMouseCursorDeltaX;
    dy = mMouseCursorDeltaY;

    return dx || dy;
}

} // namespace LD