#include "InputInternal.h"
#include <GLFW/glfw3.h> // hide from user
#include <Ludens/Application/Application.h>
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

    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
};

struct ApplicationObj
{
    Window window;
    RDevice rdevice;
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

void Window::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_REPEAT)
        return;

    if (action == GLFW_PRESS)
        Input::sKeyState[key] |= (PRESSED_BIT | PRESSED_THIS_FRAME_BIT);
    else if (action == GLFW_RELEASE)
        Input::sKeyState[key] = RELEASED_THIS_FRAME_BIT;
}

void Window::mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (action == GLFW_REPEAT)
        return;

    if (action == GLFW_PRESS)
        Input::sMouseState[button] |= (PRESSED_BIT | PRESSED_THIS_FRAME_BIT);
    else if (action == GLFW_RELEASE)
        Input::sMouseState[button] = RELEASED_THIS_FRAME_BIT;
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

bool Application::is_window_open()
{
    return mObj->isAlive && !glfwWindowShouldClose(mObj->window.handle);
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
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window.handle = glfwCreateWindow((int)appI.width, (int)appI.height, appI.name, nullptr, nullptr);
    window.width = appI.width;
    window.height = appI.height;
    glfwSetKeyCallback(window.handle, &Window::key_callback);
    glfwSetMouseButtonCallback(window.handle, &Window::mouse_button_callback);

    RDeviceInfo rdeviceI{
        .backend = RDEVICE_BACKEND_VULKAN,
        .window = window.handle,
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