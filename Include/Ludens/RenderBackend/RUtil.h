#pragma once

#include <Ludens/RenderBackend/RBackend.h>
#include <type_traits>

namespace LD {
namespace RUtil {

inline RImageInfo make_2d_image_info(RImageUsageFlags usage, RFormat format, uint32_t width, uint32_t height, RSamplerInfo sampler = {})
{
    return {
        .usage = usage,
        .type = RIMAGE_TYPE_2D,
        .samples = RSAMPLE_COUNT_1_BIT,
        .format = format,
        .layers = 1,
        .width = width,
        .height = height,
        .depth = 1,
        .sampler = sampler,
    };
}

inline RImageInfo make_cube_image_info(RImageUsageFlags usage, RFormat format, uint32_t size, RSamplerInfo sampler = {})
{
    return {
        .usage = usage,
        .type = RIMAGE_TYPE_CUBE,
        .samples = RSAMPLE_COUNT_1_BIT,
        .format = format,
        .layers = 6,
        .width = size,
        .height = size,
        .depth = 1,
        .sampler = sampler,
    };
}

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

template <typename T>
inline RClearColorValue make_clear_color(T r, T g, T b, T a)
{
    RClearColorValue value;
    if constexpr (std::is_same_v<T, float>)
    {
        value.float32[0] = r;
        value.float32[1] = g;
        value.float32[2] = b;
        value.float32[3] = a;
    }
    else if constexpr (std::is_same_v<T, uint32_t>)
    {
        value.uint32[0] = r;
        value.uint32[1] = g;
        value.uint32[2] = b;
        value.uint32[3] = a;
    }
    return value;
}

inline RSetBufferUpdateInfo make_single_set_buffer_udpate_info(RSet set, uint32_t dstBinding, RBindingType bindingType, RBuffer* buffer)
{
    RSetBufferUpdateInfo updateI{};
    updateI.set = set;
    updateI.dstBinding = dstBinding;
    updateI.dstArrayIndex = 0;
    updateI.bufferCount = 1;
    updateI.bufferBindingType = bindingType;
    updateI.buffers = buffer;
    return updateI;
}

inline RSetImageUpdateInfo make_single_set_image_update_info(RSet set, uint32_t dstBinding, RBindingType bindingType, RImageLayout* imageLayout, RImage* image)
{
    RSetImageUpdateInfo updateI{};
    updateI.set = set;
    updateI.dstBinding = dstBinding;
    updateI.dstArrayIndex = 0;
    updateI.imageBindingType = bindingType;
    updateI.imageCount = 1;
    updateI.imageLayouts = imageLayout;
    updateI.images = image;
    return updateI;
}

inline RImageMemoryBarrier make_image_memory_barrier(RImage image, RImageLayout oldLayout, RImageLayout newLayout, RAccessFlags srcAccess, RAccessFlags dstAccess)
{
    return {
        .image = image,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcAccess = srcAccess,
        .dstAccess = dstAccess,
    };
}

inline RBufferMemoryBarrier make_buffer_memory_barrier(RBuffer buffer, RAccessFlags srcAccess, RAccessFlags dstAccess)
{
    return {
        .buffer = buffer,
        .srcAccess = srcAccess,
        .dstAccess = dstAccess,
    };
}

} // namespace RUtil
} // namespace LD