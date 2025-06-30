#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/RenderBackend/RBackend.h>

namespace LD {

/// @brief pipeline for rendering skybox
struct SkyboxPipeline : Handle<struct SkyboxPipelineObj>
{
    static SkyboxPipeline create(RDevice device);
    static void destroy(SkyboxPipeline pipeline);

    RPipeline handle();
};

} // namespace LD