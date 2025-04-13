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
    RFormat format;
    VkFormat vkFormat;
} formatTable[] = {
    { RFORMAT_UNDEFINED, VK_FORMAT_UNDEFINED },
    { RFORMAT_BGRA8,     VK_FORMAT_B8G8R8A8_UNORM },
    { RFORMAT_RGBA8,     VK_FORMAT_R8G8B8A8_UNORM },
};
// clang-format on

void cast_format_vk(const RFormat& inFormat, VkFormat& outFormat)
{
    outFormat = formatTable[(int)inFormat].vkFormat;
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
    { RIMAGE_LAYOUT_UNDEFINED,                VK_IMAGE_LAYOUT_UNDEFINED },
    { RIMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, },
    { RIMAGE_LAYOUT_PRESENT_SRC,              VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, },
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
    RPipelineStageBits bit;
    VkPipelineStageFlagBits vkBit;
} pipelineStageBitsTable[] = {
    { RPIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT },
};
// clang-format on

void cast_pipeline_stage_bits_vk(const RPipelineStageBits& inBits, VkPipelineStageFlags& outBits)
{
    outBits = VK_PIPELINE_STAGE_NONE;

    for (uint32_t bit = 1, i = 0; bit < RPIPELINE_STAGE_BITS_ENUM_LAST_BIT; bit <<= 1, i++)
    {
        if (inBits & bit)
            outBits |= pipelineStageBitsTable[i].vkBit;
    }
}

// clang-format off
struct
{
    RAccessBits bit;
    VkAccessFlagBits vkBit;
} accessBitsTable[] = {
    { RACCESS_COLOR_ATTACHMENT_READ_BIT,  VK_ACCESS_COLOR_ATTACHMENT_READ_BIT },
    { RACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT },
};
// clang-format on

void cast_access_bits_vk(const RAccessBits& inBits, VkAccessFlags& outBits)
{
    outBits = VK_ACCESS_NONE;

    for (uint32_t bit = 1, i = 0; bit < RACCESS_BITS_ENUM_LAST_BIT; bit <<= 1, i++)
    {
        if (inBits & bit)
            outBits |= accessBitsTable[i].vkBit;
    }
}

void cast_pass_dependency_vk(const RPassDependency& inDep, const uint32_t inSrcSubpass, const uint32_t inDstSubpass, VkSubpassDependency& outDep)
{
    outDep.dependencyFlags = 0;
    outDep.srcSubpass = inSrcSubpass;
    outDep.dstSubpass = inDstSubpass;
    cast_pipeline_stage_bits_vk(inDep.srcStageMask, outDep.srcStageMask);
    cast_pipeline_stage_bits_vk(inDep.dstStageMask, outDep.dstStageMask);
    cast_access_bits_vk(inDep.srcAccessMask, outDep.srcAccessMask);
    cast_access_bits_vk(inDep.dstAccessMask, outDep.dstAccessMask);
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