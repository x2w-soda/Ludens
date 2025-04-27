#include "InputInternal.h"
#include <GLFW/glfw3.h> // hide from user
#include <Ludens/Application/Application.h>
#include <Ludens/Application/Input.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/RenderBackend/RBackend.h>

namespace LD {

struct Window
{
    GLFWwindow* handle;
    uint32_t width;
    uint32_t height;
    RDevice rdevice;

    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
};

// Input.cpp
namespace Input {
extern uint8_t sKeyState[];
} // namespace Input

void Window::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_REPEAT)
        return;

    if (action == GLFW_PRESS)
        Input::sKeyState[key] |= (KEY_PRESSED_BIT | KEY_PRESSED_THIS_FRAME_BIT);
    else if (action == GLFW_RELEASE)
        Input::sKeyState[key] = KEY_RELEASED_THIS_FRAME_BIT;
}

Application::Application(const ApplicationInfo& appI)
{
    int result = glfwInit();
    LD_ASSERT(result == GLFW_TRUE);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    mWindow = new Window();
    mWindow->handle = glfwCreateWindow((int)appI.width, (int)appI.height, appI.name, nullptr, nullptr);
    mWindow->width = appI.width;
    mWindow->height = appI.height;
    glfwSetKeyCallback(mWindow->handle, &Window::key_callback);

    RDeviceInfo rdeviceI{
        .backend = RDEVICE_BACKEND_VULKAN,
        .window = mWindow->handle,
    };
    mWindow->rdevice = RDevice::create(rdeviceI);
}

Application::~Application()
{
    RDevice::destroy(mWindow->rdevice);

    glfwDestroyWindow(mWindow->handle);

    delete mWindow;
    mWindow = nullptr;

    glfwTerminate();
}

uint32_t Application::width() const
{
    return mWindow ? mWindow->width : 0;
}

uint32_t Application::height() const
{
    return mWindow ? mWindow->height : 0;
}

float Application::aspect_ratio() const
{
    return mWindow ? (float)mWindow->width / (float)mWindow->height : 0.0f;
}

bool Application::is_window_open()
{
    return !glfwWindowShouldClose(mWindow->handle);
}

void Application::poll_events()
{
    Input::frame_boundary();

    glfwPollEvents();
}

RDevice Application::get_rdevice()
{
    return mWindow->rdevice;
}

} // namespace LD