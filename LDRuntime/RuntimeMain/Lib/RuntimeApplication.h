#pragma once

#include <Ludens/Event/Event.h>
#include <Ludens/Project/ProjectContext.h>
#include <LudensRuntime/RuntimeContext/RuntimeContext.h>

namespace LD {

struct RuntimeApplicationInfo
{
    FS::Path projectSchemaPath;
};

class RuntimeApplication
{
public:
    RuntimeApplication();
    RuntimeApplication(const RuntimeApplication&) = delete;
    ~RuntimeApplication();

    RuntimeApplication& operator=(const RuntimeApplication&) = delete;

    bool startup(const RuntimeApplicationInfo& info, std::string& err);
    void cleanup();

    void run();

private:
    RuntimeContext mRuntimeCtx = {};
};

} // namespace LD