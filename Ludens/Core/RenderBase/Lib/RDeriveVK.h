#pragma once

#include "Core/RenderBase/Include/RBinding.h"
#include "Core/RenderBase/Include/RPipeline.h"
#include "Core/RenderBase/Include/RTexture.h"
#include "Core/RenderBase/Include/VK/VKPipeline.h"
#include "Core/RenderBase/Include/VK/VKVertex.h"
#include <vulkan/vulkan_core.h>

namespace LD {

inline VkIndexType DeriveVKIndexType(RIndexType type)
{
    switch (type)
    {
    case RIndexType::u32:
        return VK_INDEX_TYPE_UINT32;
    case RIndexType::u16:
        return VK_INDEX_TYPE_UINT16;
    default:
        break;
    }

    LD_DEBUG_UNREACHABLE;
}

inline VkDescriptorType DeriveVKDescriptorType(RBindingType type)
{
    switch (type)
    {
    case RBindingType::Texture:
        return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    case RBindingType::UniformBuffer:
        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    default:
        break;
    }

    LD_DEBUG_UNREACHABLE;
}

inline VkPrimitiveTopology DeriveVKPrimitiveTopology(RPrimitiveTopology topology)
{
    switch (topology)
    {
    case RPrimitiveTopology::TriangleList:
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    case RPrimitiveTopology::LineList:
        return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    default:
        break;
    }

    LD_DEBUG_UNREACHABLE;
}

inline VkBlendOp DeriveVKBlendOp(RBlendMode mode)
{
    switch (mode)
    {
    case RBlendMode::Add:
        return VK_BLEND_OP_ADD;
    default:
        break;
    }

    LD_DEBUG_UNREACHABLE;
}

inline VkCompareOp DeriveVKCompareOp(RCompareMode mode)
{
    switch (mode)
    {
    case RCompareMode::Less:
        return VK_COMPARE_OP_LESS;
    case RCompareMode::LessEqual:
        return VK_COMPARE_OP_LESS_OR_EQUAL;
    default:
        break;
    }

    LD_DEBUG_UNREACHABLE;
}

inline VkBlendFactor DeriveVKBlendFactor(RBlendFactor factor)
{
    switch (factor)
    {
    case RBlendFactor::Zero:
        return VK_BLEND_FACTOR_ZERO;
    case RBlendFactor::One:
        return VK_BLEND_FACTOR_ONE;
    case RBlendFactor::SrcAlpha:
        return VK_BLEND_FACTOR_SRC_ALPHA;
    case RBlendFactor::DstAlpha:
        return VK_BLEND_FACTOR_DST_ALPHA;
    case RBlendFactor::OneMinusSrcAlpha:
        return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    case RBlendFactor::OneMinusDstAlpha:
        return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
    default:
        break;
    }

    LD_DEBUG_UNREACHABLE;
}

void DeriveVKSamplerAddressMode(const RSamplerAddressMode& inAddrMode, VkSamplerAddressMode& outAddrMode);
void DeriveVKSamplerFilter(const RSamplerFilter& inFilter, VkFilter& outFilter);
void DeriveVKAttachmentDescription(const RPassAttachment& inAttachment, VkAttachmentDescription& outAttachment);
void DeriveVKVertexLayout(const RVertexLayout& inLayout, VKVertexLayout& outLayout);
VkFormat DeriveVKImageFormat(RTextureFormat format);
RTextureFormat DeriveRTextureFormat(VkFormat format);

} // namespace LD