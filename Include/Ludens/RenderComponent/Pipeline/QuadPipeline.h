#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/Header/Math/Mat4.h>
#include <Ludens/RenderBackend/RBackend.h>

namespace LD {

/// @brief Pipeline for rendering QuadVertex, this is the backbone for screen space rendering.
struct QuadPipeline : Handle<struct QuadPipelineObj>
{
    struct PushConstant
    {
        uint32_t vpIndex; /// index into view projection data array
    };

    static QuadPipeline create(RDevice device);
    static void destroy(QuadPipeline pipeline);
    static RSetLayoutInfo image_slot_set_layout();
    static RPipelineLayoutInfo layout();
    static constexpr size_t image_slots() { return 8; }

    RPipeline handle();
};

} // namespace LD