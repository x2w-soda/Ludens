#include "InputInternal.h"
#include <GLFW/glfw3.h> // hide from user
#include <Ludens/Application/Application.h>
#include <Ludens/Application/Event.h>
#include <Ludens/Application/Input.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Log/Log.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/System/Memory.h>
#include <Ludens/System/Timer.h>

namespace LD {

static Log sLog("Application");

struct Window
{
    GLFWwindow* handle;
    uint32_t width;
    uint32_t height;

    static void size_callback(GLFWwindow* window, int width, int height);
    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
    static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos);
};

struct ApplicationObj
{
    Window window;
    RDevice rdevice;
    void* user;
    void (*onEvent)(const Event* event, void* user);
    bool isAlive;

    ApplicationObj() = delete;
    ApplicationObj(const ApplicationObj&) = delete;
    ApplicationObj(const ApplicationInfo& appI);
    ~ApplicationObj();

    ApplicationObj& operator=(const ApplicationObj&) = delete;
};

static ApplicationObj* sAppInstance = nullptr;

// Input.cpp
namespace Input {
extern uint8_t sKeyState[];
extern uint8_t sMouseState[];
extern float sMouseCursorDeltaX;
extern float sMouseCursorDeltaY;
extern float sMouseCursorX;
extern float sMouseCursorY;
} // namespace Input

void Window::size_callback(GLFWwindow* window, int width, int height)
{
    Window* w = (Window*)glfwGetWindowUserPointer(window);

    w->width = width;
    w->height = height;

    ApplicationResizeEvent event(width, height);
    Application::on_event(&event);
}

void Window::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_REPEAT)
        return;

    if (action == GLFW_PRESS)
    {
        Input::sKeyState[key] |= (PRESSED_BIT | PRESSED_THIS_FRAME_BIT);

        KeyDownEvent event((KeyCode)key);
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

Application::Application(ApplicationObj* obj)
    : mObj(obj)
{
}

Application Application::create(const ApplicationInfo& appI)
{
    LD_PROFILE_SCOPE;

    LD_ASSERT(sAppInstance == nullptr && "Application is a singleton");
    sAppInstance = heap_new<ApplicationObj>(MEMORY_USAGE_MISC, appI);
    sAppInstance->onEvent = appI.onEvent;
    sAppInstance->user = appI.user;

    return {sAppInstance};
}

void Application::destroy()
{
    LD_PROFILE_SCOPE;

    heap_delete<ApplicationObj>(sAppInstance);
    sAppInstance = nullptr;
}

uint32_t Application::width() const
{
    return mObj->window.width;
}

uint32_t Application::height() const
{
    return mObj->window.height;
}

float Application::aspect_ratio() const
{
    return (float)mObj->window.width / (float)mObj->window.height;
}

bool Application::is_window_minimized()
{
    return mObj->window.width == 0 || mObj->window.height == 0;
}

bool Application::is_window_open()
{
    return mObj->isAlive && !glfwWindowShouldClose(mObj->window.handle);
}

void Application::set_window_title(const char* cstr)
{
    glfwSetWindowTitle(mObj->window.handle, cstr);
}

void Application::poll_events()
{
    LD_PROFILE_SCOPE;

    Input::frame_boundary();

    double xpos, ypos;
    glfwGetCursorPos(mObj->window.handle, &xpos, &ypos);

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

    glfwPollEvents();
}

RDevice Application::get_rdevice()
{
    return mObj->rdevice;
}

Application Application::get()
{
    return {sAppInstance};
}

void Application::on_event(const Event* event)
{
    if (!sAppInstance->onEvent)
        return;

    sAppInstance->onEvent(event, sAppInstance->user);
}

double Application::get_time()
{
    return glfwGetTime();
}

void Application::exit()
{
    mObj->isAlive = false;
}

void Application::set_cursor_mode_normal()
{
    GLFWwindow* wh = mObj->window.handle;

    glfwSetInputMode(wh, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    double xpos, ypos;
    glfwGetCursorPos(wh, &xpos, &ypos);

    Input::sMouseCursorDeltaX = 0.0f;
    Input::sMouseCursorDeltaY = 0.0f;
    Input::sMouseCursorX = (float)xpos;
    Input::sMouseCursorY = (float)ypos;
}

void Application::set_cursor_mode_disabled()
{
    GLFWwindow* wh = mObj->window.handle;

    glfwSetInputMode(wh, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

ApplicationObj::ApplicationObj(const ApplicationInfo& appI)
    : isAlive(true)
{
    Timer timer;
    timer.start();

    int result = glfwInit();
    LD_ASSERT(result == GLFW_TRUE);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window.handle = glfwCreateWindow((int)appI.width, (int)appI.height, appI.name, nullptr, nullptr);
    window.width = appI.width;
    window.height = appI.height;
    glfwSetWindowUserPointer(window.handle, &window);
    glfwSetWindowSizeCallback(window.handle, &Window::size_callback);
    glfwSetKeyCallback(window.handle, &Window::key_callback);
    glfwSetMouseButtonCallback(window.handle, &Window::mouse_button_callback);
    glfwSetCursorPosCallback(window.handle, &Window::cursor_pos_callback);

    RDeviceInfo rdeviceI{
        .backend = RDEVICE_BACKEND_VULKAN,
        .window = window.handle,
        .vsync = appI.vsync,
    };
    rdevice = RDevice::create(rdeviceI);

    sLog.info("application ctor {:.3f}s", timer.stop() / 1000000.0f);
}

ApplicationObj::~ApplicationObj()
{
    Timer timer;
    timer.start();

    RDevice::destroy(rdevice);

    glfwDestroyWindow(window.handle);
    glfwTerminate();

    sLog.info("application dtor {:.3f}s", timer.stop() / 1000000.0f);
}

} // namespace LD