#include <Ludens/JobSystem/JobSystem.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/WindowRegistry/WindowRegistry.h>

#include "RuntimeApplication.h"

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
    WindowRegistry::create(windowI);

    RuntimeContextInfo ctxI{};
    ctxI.project = project;
    mRuntimeCtx = RuntimeContext::create(ctxI);
}

RuntimeApplication::~RuntimeApplication()
{
    LD_PROFILE_SCOPE;

    RuntimeContext::destroy(mRuntimeCtx);
    WindowRegistry::destroy();
    JobSystem::shutdown();
}

void RuntimeApplication::run()
{
    WindowRegistry reg = WindowRegistry::get();
    WindowID rootID = reg.get_root_id();

    while (reg.is_window_open(rootID))
    {
        reg.poll_events();

        if (reg.is_window_minimized(rootID))
            continue;

        float delta = (float)reg.get_delta_time();

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