#pragma once

#include <Ludens/Event/Event.h>
#include <Ludens/Project/Project.h>
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
    RuntimeContext mRuntimeCtx;
};

} // namespace LD