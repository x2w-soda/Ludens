#include <Ludens/JobSystem/JobSystem.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/WindowRegistry/WindowRegistry.h>

#include "RuntimeApplication.h"

namespace LD {

RuntimeApplication::RuntimeApplication()
{
    LD_PROFILE_SCOPE;

    JobSystemInfo jsI{};
    jsI.immediateQueueCapacity = 128;
    jsI.standardQueueCapacity = 128;
    JobSystem::init(jsI);
}

RuntimeApplication::~RuntimeApplication()
{
    LD_PROFILE_SCOPE;

    if (mRuntimeCtx)
        RuntimeContext::destroy(mRuntimeCtx);

    JobSystem::shutdown();
}

bool RuntimeApplication::startup(const RuntimeApplicationInfo& info, std::string& err)
{
    RuntimeContextInfo ctxI{};
    ctxI.projectSchemaPath = info.projectSchemaPath;
    mRuntimeCtx = RuntimeContext::create(ctxI);

    if (!mRuntimeCtx)
    {
        err = "failed to create RuntimeContext";
        return false;
    }

    return true;
}

void RuntimeApplication::cleanup()
{
    if (mRuntimeCtx)
    {
        RuntimeContext::destroy(mRuntimeCtx);
        mRuntimeCtx = {};
    }
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

} // namespace LD