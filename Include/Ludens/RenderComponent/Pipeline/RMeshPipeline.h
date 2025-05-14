#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/RenderBackend/RBackend.h>

namespace LD {

struct RMeshBlinnPhongPipeline : Handle<struct RMeshBlinnPhongPipelineObj>
{
    static RMeshBlinnPhongPipeline create(RDevice device);
    static void destroy(RMeshBlinnPhongPipeline pipeline);

    RPipeline handle();
};

} // namespace LD