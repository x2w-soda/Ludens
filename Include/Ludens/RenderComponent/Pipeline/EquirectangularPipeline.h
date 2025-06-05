#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/RenderBackend/RBackend.h>

namespace LD {

struct EquirectangularPipeline : Handle<struct EquirectangularPipelineObj>
{
    static EquirectangularPipeline create(RDevice device);
    static void destroy(EquirectangularPipeline pipeline);

    RPipeline handle();
};

/// @brief One-shot function to render 6 cube faces from a 2D equirectangular image,
///        submits work to the graphics queue and blocks until completion.
/// @param device render device
/// @param pipeline equirectangular pipeline
/// @param srcImage the equirectangular image to sample from
/// @param dstImages the 6 cubemap faces used as render targets
/// @param dstBuffers if not null, the 6 images are also copied to these (most likely host-visible) 6 buffers.
///                   The dstImages are then expected to contain RIMAGE_USAGE_TRANSFER_SRC_BIT.
void equirectangular_cmd_render_to_faces(RDevice device, EquirectangularPipeline pipeline, RImage srcImage, RImage* dstImages, RBuffer* dstBuffers);

} // namespace LD