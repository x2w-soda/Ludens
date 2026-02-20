#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/RenderBackend/RBackend.h>

namespace LD {

struct LinePipeline : Handle<struct LinePipelineObj>
{
    static LinePipeline create(RDevice device);
    static void destroy(LinePipeline pipeline);

    struct PushConstant
    {
        uint32_t vpIndex; /// index into view projection data array
    };

    RPipeline handle();
};

} // namespace LD