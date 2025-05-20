#pragma once

#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/Header/Handle.h>

namespace LD {

struct OutlinePipeline : Handle<struct OutlinePipelineObj>
{
    static OutlinePipeline create(RDevice device);
    static void destroy(OutlinePipeline pipeline);

    static RPipelineLayoutInfo get_layout();

    RPipeline handle();
};

} // namespace LD