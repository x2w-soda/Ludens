#include <GLFW/glfw3.h> // hide from user
#include <Ludens/Application/Application.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/RenderBackend/RFactory.h>

namespace LD {

struct Window
{
    GLFWwindow* handle;
    uint32_t width;
    uint32_t height;
    RDevice rdevice;
};

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

bool Application::is_window_open()
{
    return !glfwWindowShouldClose(mWindow->handle);
}

void Application::poll_events()
{
    glfwPollEvents();
}

RDevice Application::get_rdevice()
{
    return mWindow->rdevice;
}

} // namespace LD