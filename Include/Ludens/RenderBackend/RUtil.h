#pragma once

#include <Ludens/RenderBackend/RBackend.h>

namespace LD {
namespace RUtil {

/// @brief Creates a 'default' blend state, using the alpha channel to linearly interpolate colors.
///        dstAlpha = srcAlpha,
///        dstColor = srcAlpha * srcColor + (1 - srcAlpha) * dstColor
inline RPipelineBlendState make_default_blend_state()
{
    return {
        .enabled = true,
        .srcColorFactor = RBLEND_FACTOR_SRC_ALPHA,
        .dstColorFactor = RBLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .srcAlphaFactor = RBLEND_FACTOR_ONE,
        .dstAlphaFactor = RBLEND_FACTOR_ZERO,
        .colorBlendOp = RBLEND_OP_ADD,
        .alphaBlendOp = RBLEND_OP_ADD,
    };
}

} // namespace RUtil
} // namespace LD