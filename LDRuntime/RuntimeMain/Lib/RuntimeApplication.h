#pragma once

#include <Ludens/Event/Event.h>
#include <Ludens/Project/Project.h>
#include <Ludens/Window/Window.h>
#include <LudensRuntime/RuntimeContext/RuntimeContext.h>

namespace LD {

class RuntimeApplication
{
public:
    RuntimeApplication() = delete;
    RuntimeApplication(Project project);
    RuntimeApplication(const RuntimeApplication&) = delete;
    ~RuntimeApplication();

    RuntimeApplication& operator=(const RuntimeApplication&) = delete;

    void run();

private:
    static void on_event(const Event* event, void* user);

    RuntimeContext mRuntimeCtx;
};

} // namespace LD