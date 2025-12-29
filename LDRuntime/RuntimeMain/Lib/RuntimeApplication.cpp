#include "RuntimeApplication.h"
#include <Ludens/JobSystem/JobSystem.h>
#include <Ludens/Profiler/Profiler.h>

namespace LD {

RuntimeApplication::RuntimeApplication(Project project)
{
    LD_PROFILE_SCOPE;

    ProjectStartupSettings startupS = project.get_settings().get_startup_settings();
    std::string windowName = startupS.get_window_name();

    JobSystemInfo jsI{};
    jsI.immediateQueueCapacity = 128;
    jsI.standardQueueCapacity = 128;
    JobSystem::init(jsI);

    WindowInfo windowI{};
    windowI.width = startupS.get_window_width();
    windowI.height = startupS.get_window_height();
    windowI.name = windowName.c_str();
    windowI.onEvent = &RuntimeApplication::on_event;
    windowI.user = this;
    windowI.hintBorderColor = 0;
    windowI.hintTitleBarColor = 0;
    windowI.hintTitleBarTextColor = 0;
    Window::create(windowI);

    RuntimeContextInfo ctxI{};
    ctxI.project = project;
    mRuntimeCtx = RuntimeContext::create(ctxI);
}

RuntimeApplication::~RuntimeApplication()
{
    LD_PROFILE_SCOPE;

    RuntimeContext::destroy(mRuntimeCtx);
    Window::destroy(Window::get());
    JobSystem::shutdown();
}

void RuntimeApplication::run()
{
    Window window = Window::get();

    while (window.is_open())
    {
        window.poll_events();

        if (window.is_minimized())
            continue;

        float delta = (float)window.get_delta_time();
        const Vec2 screenExtent = window.extent();

        mRuntimeCtx.update(delta);

        LD_PROFILE_FRAME_MARK;
    }
}

void RuntimeApplication::on_event(const Event* event, void* user)
{
    auto& self = *(RuntimeApplication*)user;

    // TODO:
}

} // namespace LD