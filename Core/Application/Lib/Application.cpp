#include "InputInternal.h"
#include "Window.h"
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

/// @brief Application implementation.
struct ApplicationObj
{
    Window window;
    RDevice rdevice;
    void* user;
    void (*onEvent)(const Event* event, void* user);
    bool isAlive;
    double timeDelta;
    double timePrevFrame;
    double timeThisFrame;

    ApplicationObj() = delete;
    ApplicationObj(const ApplicationObj&) = delete;
    ApplicationObj(const ApplicationInfo& appI);
    ~ApplicationObj();

    ApplicationObj& operator=(const ApplicationObj&) = delete;

    void frame_boundary();
};

static ApplicationObj* sAppInstance = nullptr;

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
    return mObj->window.width();
}

uint32_t Application::height() const
{
    return mObj->window.height();
}

float Application::aspect_ratio() const
{
    return mObj->window.aspect_ratio();
}

bool Application::is_window_minimized()
{
    return mObj->window.width() == 0 || mObj->window.height() == 0;
}

bool Application::is_window_open()
{
    return mObj->isAlive && mObj->window.is_open();
}

void Application::poll_events()
{
    LD_PROFILE_SCOPE;

    // updates application delta time
    mObj->frame_boundary();

    // updates input state for polling
    Input::frame_boundary();

    double xpos, ypos;
    mObj->window.get_cursor_pos(xpos, ypos);

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

    mObj->window.poll_events();
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
    return mObj->window.get_time();
}

double Application::get_delta_time()
{
    return mObj->timeDelta;
}

void Application::exit()
{
    mObj->isAlive = false;
}

void Application::set_cursor_mode_normal()
{
    mObj->window.set_cursor_mode_normal();

    double xpos, ypos;
    mObj->window.get_cursor_pos(xpos, ypos);

    Input::sMouseCursorDeltaX = 0.0f;
    Input::sMouseCursorDeltaY = 0.0f;
    Input::sMouseCursorX = (float)xpos;
    Input::sMouseCursorY = (float)ypos;
}

void Application::set_cursor_mode_disabled()
{
    mObj->window.set_cursor_mode_disabled();
}

void Application::hint_border_color(Color color)
{
    mObj->window.hint_border_color(color);
}

void Application::hint_title_bar_color(Color color)
{
    mObj->window.hint_title_bar_color(color);
}

void Application::hint_title_bar_text_color(Color color)
{
    mObj->window.hint_title_bar_text_color(color);
}

void Application::hint_title_bar_text(const char* cstr)
{
    if (!cstr)
        return;

    mObj->window.hint_title_bar_text(cstr);
}

void Application::hint_cursor_shape(CursorType cursor)
{
    mObj->window.hint_cursor_shape(cursor);
}

ApplicationObj::ApplicationObj(const ApplicationInfo& appI)
    : isAlive(true)
{
    Timer timer;
    timer.start();

    window.startup(appI);

    RDeviceInfo rdeviceI{
        .backend = RDEVICE_BACKEND_VULKAN,
        .window = window.get_glfw_handle(),
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

    window.cleanup();

    sLog.info("application dtor {:.3f}s", timer.stop() / 1000000.0f);
}

void ApplicationObj::frame_boundary()
{
    static bool sIsFirstFrame = true;

    if (sIsFirstFrame)
    {
        sIsFirstFrame = false;
        timePrevFrame = window.get_time();
    }

    timeThisFrame = window.get_time();
    timeDelta = timeThisFrame - timePrevFrame;
    timePrevFrame = timeThisFrame;
}

} // namespace LD