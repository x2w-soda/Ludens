#pragma once

#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/Header/Handle.h>

namespace LD {

struct LinePipeline : Handle<struct LinePipelineObj>
{
    static LinePipeline create(RDevice device);
    static void destroy(LinePipeline pipeline);

    RPipeline handle();
};

} // namespace LD