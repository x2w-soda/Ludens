#include "RUtil.h"
#include <Ludens/Header/Assert.h>
#include <cstring>

// clang-format off
#define KASE(E, V) case E: V = #E; break
// clang-format on

namespace LD {
namespace RUtil {

void cast_clear_color_value_vk(const RClearColorValue& inValue, VkClearColorValue& outValue)
{
    static_assert(sizeof(RClearColorValue) == sizeof(VkClearColorValue));

    memcpy(&outValue, &inValue, sizeof(RClearColorValue));
}

// clang-format off
struct
{
    RFilter filter;
    VkFilter vkFilter;
    VkSamplerMipmapMode vkMipmapMode;
} filterTable[] = {
    { RFILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST },
    { RFILTER_LINEAR,  VK_FILTER_LINEAR,  VK_SAMPLER_MIPMAP_MODE_LINEAR },
};
// clang-format on

void cast_filter_vk(const RFilter& inFilter, VkFilter& outFilter)
{
    outFilter = filterTable[(int)inFilter].vkFilter;
}

void cast_filter_mipmap_mode_vk(const RFilter& inFilter, VkSamplerMipmapMode& outMipmapMode)
{
    outMipmapMode = filterTable[(int)inFilter].vkMipmapMode;
}

// clang-format off
struct
{
    RSamplerAddressMode mode;
    VkSamplerAddressMode vkMode;
} samplerAddressModeTable[] = {
    { RSAMPLER_ADDRESS_MODE_REPEAT,           VK_SAMPLER_ADDRESS_MODE_REPEAT },
    { RSAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,  VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT },
    { RSAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,    VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE },
};
// clang-format on

void cast_sampler_address_mode_vk(const RSamplerAddressMode& inMode, VkSamplerAddressMode& outMode)
{
    outMode = samplerAddressModeTable[(int)inMode].vkMode;
}

// clang-format off
struct
{
    RFormat format;
    uint32_t texelSize;
    VkFormat vkFormat;
    VkImageAspectFlags vkImageAspects;
} formatTable[] = {
    { RFORMAT_UNDEFINED, 0, VK_FORMAT_UNDEFINED,        (VkImageAspectFlags)0,      },
    { RFORMAT_BGRA8,     4, VK_FORMAT_B8G8R8A8_UNORM,    VK_IMAGE_ASPECT_COLOR_BIT, },
    { RFORMAT_RGBA8,     4, VK_FORMAT_R8G8B8A8_UNORM,    VK_IMAGE_ASPECT_COLOR_BIT, },
};
// clang-format on

void cast_format_vk(const RFormat& inFormat, VkFormat& outFormat)
{
    outFormat = formatTable[(int)inFormat].vkFormat;
}

void cast_format_image_aspect_vk(const RFormat& inFormat, VkImageAspectFlags& outFlags)
{
    outFlags = formatTable[(int)inFormat].vkImageAspects;
}

uint32_t get_format_texel_size(const RFormat& format)
{
    return formatTable[(int)format].texelSize;
}

// clang-format off
struct
{
    RGLSLType type;
    VkFormat vkFormat;
} glslTypeTable[] = {
    { RGLSL_TYPE_FLOAT,  VK_FORMAT_R32_SFLOAT },
    { RGLSL_TYPE_VEC2,   VK_FORMAT_R32G32_SFLOAT },
    { RGLSL_TYPE_VEC3,   VK_FORMAT_R32G32B32_SFLOAT },
    { RGLSL_TYPE_VEC4,   VK_FORMAT_R32G32B32A32_SFLOAT },
};
// clang-format on

void cast_glsl_type_vk(const RGLSLType& inType, VkFormat& outFormat)
{
    outFormat = glslTypeTable[(int)inType].vkFormat;
}

// clang-format off
struct
{
    RImageLayout imageLayout;
    VkImageLayout vkImageLayout;
} imageLayoutTable[] = {
    { RIMAGE_LAYOUT_UNDEFINED,         VK_IMAGE_LAYOUT_UNDEFINED },
    { RIMAGE_LAYOUT_COLOR_ATTACHMENT,  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, },
    { RIMAGE_LAYOUT_PRESENT_SRC,       VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, },
    { RIMAGE_LAYOUT_SHADER_READ_ONLY,  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, },
    { RIMAGE_LAYOUT_TRANSFER_SRC,      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, },
    { RIMAGE_LAYOUT_TRANSFER_DST,      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, },
};
// clang-format on

void cast_image_layout_vk(const RImageLayout& inLayout, VkImageLayout& outLayout)
{
    outLayout = imageLayoutTable[(int)inLayout].vkImageLayout;
}

// clang-format off
struct
{
    RAttachmentLoadOp loadOp;
    VkAttachmentLoadOp vkLoadOp;
} attachmentLoadOpTable[] = {
    { RATTACHMENT_LOAD_OP_LOAD,      VK_ATTACHMENT_LOAD_OP_LOAD },
    { RATTACHMENT_LOAD_OP_CLEAR,     VK_ATTACHMENT_LOAD_OP_CLEAR },
    { RATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_LOAD_OP_DONT_CARE },
};
// clang-format on

void cast_attachment_load_op_vk(const RAttachmentLoadOp& inOp, VkAttachmentLoadOp& outOp)
{
    outOp = attachmentLoadOpTable[(int)inOp].vkLoadOp;
}

// clang-format off
struct
{
    RAttachmentStoreOp storeOp;
    VkAttachmentStoreOp vkStoreOp;
} attachmentStoreOpTable[] = {
    { RATTACHMENT_STORE_OP_STORE,     VK_ATTACHMENT_STORE_OP_STORE },
    { RATTACHMENT_STORE_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE },
};
// clang-format on

void cast_attachment_store_op_vk(const RAttachmentStoreOp& inOp, VkAttachmentStoreOp& outOp)
{
    outOp = attachmentStoreOpTable[(int)inOp].vkStoreOp;
}

void cast_pass_color_attachment_vk(const RPassColorAttachment& inAttachment, VkAttachmentDescription& outDesc)
{
    outDesc.flags = 0;
    outDesc.samples = VK_SAMPLE_COUNT_1_BIT;
    outDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    outDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    cast_format_vk(inAttachment.colorFormat, outDesc.format);
    cast_attachment_load_op_vk(inAttachment.colorLoadOp, outDesc.loadOp);
    cast_attachment_store_op_vk(inAttachment.colorStoreOp, outDesc.storeOp);
    cast_image_layout_vk(inAttachment.initialLayout, outDesc.initialLayout);
    cast_image_layout_vk(inAttachment.finalLayout, outDesc.finalLayout);
}

void cast_pass_depth_stencil_attachment_vk(const RPassDepthStencilAttachment& inAttachment, VkAttachmentDescription& outDesc)
{
    outDesc.flags = 0;
    outDesc.samples = VK_SAMPLE_COUNT_1_BIT;

    cast_attachment_load_op_vk(inAttachment.depthLoadOp, outDesc.loadOp);
    cast_attachment_store_op_vk(inAttachment.depthStoreOp, outDesc.storeOp);
    cast_attachment_load_op_vk(inAttachment.stencilLoadOp, outDesc.stencilLoadOp);
    cast_attachment_store_op_vk(inAttachment.stencilStoreOp, outDesc.stencilStoreOp);
    cast_image_layout_vk(inAttachment.initialLayout, outDesc.initialLayout);
    cast_image_layout_vk(inAttachment.finalLayout, outDesc.finalLayout);
}

// clang-format off
struct
{
    RPipelineStageBit bit;
    VkPipelineStageFlagBits vkBit;
} pipelineStageBitsTable[] = {
    { RPIPELINE_STAGE_TOP_OF_PIPE_BIT,             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT },
    { RPIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT },
    { RPIPELINE_STAGE_TRANSFER_BIT,                VK_PIPELINE_STAGE_TRANSFER_BIT },
    { RPIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,          VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT },
};
// clang-format on

void cast_pipeline_stage_flags_vk(const RPipelineStageFlags& inFlags, VkPipelineStageFlags& outFlags)
{
    outFlags = VK_PIPELINE_STAGE_NONE;

    for (uint32_t bit = 1, i = 0; bit < RPIPELINE_STAGE_BITS_ENUM_LAST_BIT; bit <<= 1, i++)
    {
        if (inFlags & bit)
            outFlags |= pipelineStageBitsTable[i].vkBit;
    }
}

// clang-format off
struct
{
    RAccessBit bit;
    VkAccessFlagBits vkBit;
} accessBitsTable[] = {
    { RACCESS_COLOR_ATTACHMENT_READ_BIT,  VK_ACCESS_COLOR_ATTACHMENT_READ_BIT },
    { RACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT },
    { RACCESS_TRANSFER_READ_BIT,          VK_ACCESS_TRANSFER_READ_BIT },
    { RACCESS_TRANSFER_WRITE_BIT,         VK_ACCESS_TRANSFER_WRITE_BIT },
};
// clang-format on

void cast_access_flags_vk(const RAccessFlags& inFlags, VkAccessFlags& outFlags)
{
    outFlags = VK_ACCESS_NONE;

    for (uint32_t bit = 1, i = 0; bit < RACCESS_BITS_ENUM_LAST_BIT; bit <<= 1, i++)
    {
        if (inFlags & bit)
            outFlags |= accessBitsTable[i].vkBit;
    }
}

void cast_pass_dependency_vk(const RPassDependency& inDep, const uint32_t inSrcSubpass, const uint32_t inDstSubpass, VkSubpassDependency& outDep)
{
    outDep.dependencyFlags = 0;
    outDep.srcSubpass = inSrcSubpass;
    outDep.dstSubpass = inDstSubpass;
    cast_pipeline_stage_flags_vk(inDep.srcStageMask, outDep.srcStageMask);
    cast_pipeline_stage_flags_vk(inDep.dstStageMask, outDep.dstStageMask);
    cast_access_flags_vk(inDep.srcAccessMask, outDep.srcAccessMask);
    cast_access_flags_vk(inDep.dstAccessMask, outDep.dstAccessMask);
}

// clang-format off
struct
{
    RShaderType type;
    VkShaderStageFlagBits vkType;
} shaderTypeTable[] = {
    { RSHADER_TYPE_VERTEX_SHADER,   VK_SHADER_STAGE_VERTEX_BIT },
    { RSHADER_TYPE_FRAGMENT_SHADER, VK_SHADER_STAGE_FRAGMENT_BIT },
};
// clang-format on

void cast_shader_type_vk(const RShaderType& inType, VkShaderStageFlagBits& outType)
{
    outType = shaderTypeTable[(int)inType].vkType;
}

// clang-format off
struct
{
    RBindingType type;
    VkDescriptorType vkType;
} bindingTypeTable[] = {
    { RBINDING_TYPE_COMBINED_IMAGE_SAMPLER, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER },
};
// clang-format on

void cast_binding_type_vk(const RBindingType& inType, VkDescriptorType& outType)
{
    outType = bindingTypeTable[(int)inType].vkType;
}

void cast_set_layout_binding_vk(const RSetBindingInfo& inBinding, VkDescriptorSetLayoutBinding& outBinding)
{
    outBinding.binding = inBinding.binding;
    outBinding.pImmutableSamplers = nullptr;
    outBinding.descriptorCount = inBinding.arrayCount;
    cast_binding_type_vk(inBinding.type, outBinding.descriptorType);

    /// NOTE: we make the simplification that all vulkan descriptors may be accessed at all shader stages.
    outBinding.stageFlags = VK_SHADER_STAGE_ALL;
}

void cast_vertex_attribute_vk(const RVertexAttribute& inAttr, uint32_t inLocation, VkVertexInputAttributeDescription& outAttr)
{
    outAttr.location = inLocation;
    outAttr.binding = inAttr.binding;
    outAttr.offset = inAttr.offset;
    cast_glsl_type_vk(inAttr.type, outAttr.format);
}

void cast_vertex_binding_vk(const RVertexBinding& inBinding, uint32_t inIndex, VkVertexInputBindingDescription& outBinding)
{
    outBinding.binding = inIndex;
    outBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    outBinding.stride = inBinding.stride;
}

// clang-format off
struct
{
    RBufferUsageBit usage;
    VkBufferUsageFlagBits vkUsage;
} bufferUsageTable[] = {
    { RBUFFER_USAGE_TRANSFER_SRC_BIT, VK_BUFFER_USAGE_TRANSFER_SRC_BIT },
    { RBUFFER_USAGE_TRANSFER_DST_BIT, VK_BUFFER_USAGE_TRANSFER_DST_BIT },
    { RBUFFER_USAGE_VERTEX_BIT,       VK_BUFFER_USAGE_VERTEX_BUFFER_BIT },
    { RBUFFER_USAGE_INDEX_BIT,        VK_BUFFER_USAGE_INDEX_BUFFER_BIT },
};
// clang-format on

void cast_buffer_usage_vk(const RBufferUsageFlags& inUsage, VkBufferUsageFlags& outUsage)
{
    const int len = sizeof(bufferUsageTable) / sizeof(*bufferUsageTable);
    VkBufferUsageFlags vkUsage = 0;

    for (int i = 0; i < len; i++)
    {
        if (inUsage & bufferUsageTable[i].usage)
            vkUsage |= bufferUsageTable[i].vkUsage;
    }

    outUsage = vkUsage;
}

// clang-format off
struct
{
    RImageUsageBit usage;
    VkImageUsageFlagBits vkUsage;
} imageUsageTable[] = {
    { RIMAGE_USAGE_TRANSFER_SRC_BIT,  VK_IMAGE_USAGE_TRANSFER_SRC_BIT },
    { RIMAGE_USAGE_TRANSFER_DST_BIT,  VK_IMAGE_USAGE_TRANSFER_DST_BIT },
    { RIMAGE_USAGE_SAMPLED_BIT,       VK_IMAGE_USAGE_SAMPLED_BIT },
};
// clang-format on

void cast_image_usage_vk(const RImageUsageFlags& inUsage, VkImageUsageFlags& outUsage)
{
    const int len = sizeof(imageUsageTable) / sizeof(*imageUsageTable);
    VkImageUsageFlags vkUsage = 0;

    for (int i = 0; i < len; i++)
    {
        if (inUsage & imageUsageTable[i].usage)
            vkUsage |= imageUsageTable[i].vkUsage;
    }

    outUsage = vkUsage;
}

// clang-format off
struct
{
    RImageType type;
    VkImageType vkType;
} imageTypeTable[] = {
    { RIMAGE_TYPE_2D,  VK_IMAGE_TYPE_2D},
};
// clang-format on

void cast_image_type_vk(const RImageType& inType, VkImageType& outType)
{
    outType = imageTypeTable[(int)inType].vkType;
}

// clang-format off
struct
{
    RIndexType type;
    VkIndexType vkType;
} indexTypeTable[] = {
    { RINDEX_TYPE_U16, VK_INDEX_TYPE_UINT16 },
    { RINDEX_TYPE_U32, VK_INDEX_TYPE_UINT32 },
};
// clang-format on

void cast_index_type_vk(const RIndexType& inType, VkIndexType& outType)
{
    outType = indexTypeTable[(int)inType].vkType;
}

void print_vk_queue_flags(const VkQueueFlags& inFlags, std::string& out)
{
    out.clear();

    if (inFlags & VK_QUEUE_GRAPHICS_BIT)
        out.append("VK_QUEUE_GRAPHICS_BIT");

    if (inFlags & VK_QUEUE_TRANSFER_BIT)
    {
        if (!out.empty())
            out.append(" | ");
        out.append("VK_QUEUE_TRANSFER_BIT");
    }

    if (inFlags & VK_QUEUE_COMPUTE_BIT)
    {
        if (!out.empty())
            out.append(" | ");
        out.append("VK_QUEUE_COMPUTE_BIT");
    }

    if (inFlags & VK_QUEUE_SPARSE_BINDING_BIT)
    {
        if (!out.empty())
            out.append(" | ");
        out.append("VK_QUEUE_SPARSE_BINDING_BIT");
    }

    if (inFlags & VK_QUEUE_PROTECTED_BIT)
    {
        if (!out.empty())
            out.append(" | ");
        out.append("VK_QUEUE_PROTECTED_BIT");
    }
}

void print_vk_present_mode(const VkPresentModeKHR& inMode, std::string& out)
{
    const char* str = nullptr;

    switch (inMode)
    {
        KASE(VK_PRESENT_MODE_IMMEDIATE_KHR, str);
        KASE(VK_PRESENT_MODE_MAILBOX_KHR, str);
        KASE(VK_PRESENT_MODE_FIFO_KHR, str);
        KASE(VK_PRESENT_MODE_FIFO_RELAXED_KHR, str);
        KASE(VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR, str);
        KASE(VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR, str);
    default:
        LD_UNREACHABLE;
    }

    out = str;
}

} // namespace RUtil
} // namespace LD