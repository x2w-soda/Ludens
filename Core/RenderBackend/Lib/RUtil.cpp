#include "RBackendObj.h"
#include "RUtilInternal.h"
#include <Ludens/Header/Assert.h>
#include <cstring>

// clang-format off
#define ARRAY_SIZE(A)  (sizeof(A) / sizeof(*A))
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
    { RFORMAT_UNDEFINED, 0, VK_FORMAT_UNDEFINED,            (VkImageAspectFlags)0,     },
    { RFORMAT_R8,        1, VK_FORMAT_R8_UNORM,             VK_IMAGE_ASPECT_COLOR_BIT, },
    { RFORMAT_BGRA8,     4, VK_FORMAT_B8G8R8A8_UNORM,       VK_IMAGE_ASPECT_COLOR_BIT, },
    { RFORMAT_RGBA8,     4, VK_FORMAT_R8G8B8A8_UNORM,       VK_IMAGE_ASPECT_COLOR_BIT, },
    { RFORMAT_R32U,      4, VK_FORMAT_R32_UINT,             VK_IMAGE_ASPECT_COLOR_BIT, },
    { RFORMAT_D32F_S8U,  5, VK_FORMAT_D32_SFLOAT_S8_UINT,   VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, },
    { RFORMAT_D24_S8U,   5, VK_FORMAT_D24_UNORM_S8_UINT,    VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, },
};
// clang-format on

void cast_format_vk(const RFormat& inFormat, VkFormat& outFormat)
{
    outFormat = formatTable[(int)inFormat].vkFormat;
}

void cast_format_from_vk(const VkFormat& inFormat, RFormat& outFormat)
{
    for (size_t i = 0; i < ARRAY_SIZE(formatTable); i++)
    {
        if (formatTable[i].vkFormat == inFormat)
        {
            outFormat = formatTable[i].format;
            return;
        }
    }

    LD_UNREACHABLE;
}

void cast_format_image_aspect_vk(const RFormat& inFormat, VkImageAspectFlags& outFlags)
{
    outFlags = formatTable[(int)inFormat].vkImageAspects;
}

uint32_t get_format_texel_size(const RFormat& format)
{
    return formatTable[(int)format].texelSize;
}

void save_pass_info(const RPassInfo& inInfo, RPassInfoData& outData)
{
    outData.samples = inInfo.samples;
    outData.colorAttachmentCount = inInfo.colorAttachmentCount;
    outData.colorAttachments.resize(inInfo.colorAttachmentCount);
    outData.colorResolveAttachments.clear();
    outData.depthStencilAttachment.reset();
    outData.dependency.reset();

    std::copy(inInfo.colorAttachments, inInfo.colorAttachments + inInfo.colorAttachmentCount, outData.colorAttachments.begin());

    if (inInfo.colorResolveAttachments)
    {
        outData.colorResolveAttachments.resize(inInfo.colorAttachmentCount);
        std::copy(inInfo.colorResolveAttachments, inInfo.colorResolveAttachments + inInfo.colorAttachmentCount, outData.colorResolveAttachments.begin());
    }

    if (inInfo.depthStencilAttachment)
        outData.depthStencilAttachment = *inInfo.depthStencilAttachment;

    if (inInfo.dependency)
        outData.dependency = *inInfo.dependency;
}

void load_pass_info(const RPassInfoData& inData, RPassInfo& outInfo)
{
    outInfo.samples = inData.samples;
    outInfo.colorAttachmentCount = inData.colorAttachmentCount;
    outInfo.colorAttachments = inData.colorAttachments.data();
    outInfo.colorResolveAttachments = inData.colorResolveAttachments.empty() ? nullptr : inData.colorResolveAttachments.data();
    outInfo.depthStencilAttachment = inData.depthStencilAttachment ? std::to_address(inData.depthStencilAttachment) : nullptr;
    outInfo.dependency = inData.dependency ? std::to_address(inData.dependency) : nullptr;
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
    { RGLSL_TYPE_UINT,   VK_FORMAT_R32_UINT },
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
    { RIMAGE_LAYOUT_UNDEFINED,                 VK_IMAGE_LAYOUT_UNDEFINED },
    { RIMAGE_LAYOUT_GENERAL,                   VK_IMAGE_LAYOUT_GENERAL },
    { RIMAGE_LAYOUT_COLOR_ATTACHMENT,          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, },
    { RIMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT,  VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, },
    { RIMAGE_LAYOUT_PRESENT_SRC,               VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, },
    { RIMAGE_LAYOUT_SHADER_READ_ONLY,          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, },
    { RIMAGE_LAYOUT_TRANSFER_SRC,              VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, },
    { RIMAGE_LAYOUT_TRANSFER_DST,              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, },
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

void cast_pass_color_attachment_vk(const RPassColorAttachment& inAttachment, const RSampleCountBit& inSamples, VkAttachmentDescription& outDesc)
{
    outDesc.flags = 0;
    outDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    outDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    VkImageLayout vkPassLayout;

    cast_format_vk(inAttachment.colorFormat, outDesc.format);
    cast_sample_count_vk(inSamples, outDesc.samples);
    cast_attachment_load_op_vk(inAttachment.colorLoadOp, outDesc.loadOp);
    cast_attachment_store_op_vk(inAttachment.colorStoreOp, outDesc.storeOp);
    cast_image_layout_vk(inAttachment.initialLayout, outDesc.initialLayout);
    cast_image_layout_vk(inAttachment.passLayout, vkPassLayout);

    outDesc.finalLayout = vkPassLayout;
}

void cast_pass_color_resolve_attachment_vk(const RPassResolveAttachment& inAttachment, const RFormat& inColorFormat, VkAttachmentDescription& outDesc)
{
    outDesc.flags = 0;
    outDesc.samples = VK_SAMPLE_COUNT_1_BIT;
    outDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    outDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    VkImageLayout vkPassLayout;

    cast_format_vk(inColorFormat, outDesc.format);
    cast_attachment_load_op_vk(inAttachment.loadOp, outDesc.loadOp);
    cast_attachment_store_op_vk(inAttachment.storeOp, outDesc.storeOp);
    cast_image_layout_vk(inAttachment.initialLayout, outDesc.initialLayout);
    cast_image_layout_vk(inAttachment.passLayout, vkPassLayout);

    outDesc.finalLayout = vkPassLayout;
}

void cast_pass_depth_stencil_attachment_vk(const RPassDepthStencilAttachment& inAttachment, const RSampleCountBit& inSamples, VkAttachmentDescription& outDesc)
{
    outDesc.flags = 0;

    VkImageLayout vkPassLayout;

    cast_format_vk(inAttachment.depthStencilFormat, outDesc.format);
    cast_sample_count_vk(inSamples, outDesc.samples);
    cast_attachment_load_op_vk(inAttachment.depthLoadOp, outDesc.loadOp);
    cast_attachment_store_op_vk(inAttachment.depthStoreOp, outDesc.storeOp);
    cast_attachment_load_op_vk(inAttachment.stencilLoadOp, outDesc.stencilLoadOp);
    cast_attachment_store_op_vk(inAttachment.stencilStoreOp, outDesc.stencilStoreOp);
    cast_image_layout_vk(inAttachment.initialLayout, outDesc.initialLayout);
    cast_image_layout_vk(inAttachment.passLayout, vkPassLayout);

    outDesc.finalLayout = vkPassLayout;
}

// clang-format off
struct
{
    RPipelineStageBit bit;
    VkPipelineStageFlagBits vkBit;
} pipelineStageBitsTable[] = {
    { RPIPELINE_STAGE_TOP_OF_PIPE_BIT,             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT },
    { RPIPELINE_STAGE_DRAW_INDIRECT_BIT,           VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT },
    { RPIPELINE_STAGE_VERTEX_INPUT_BIT,            VK_PIPELINE_STAGE_VERTEX_INPUT_BIT },
    { RPIPELINE_STAGE_VERTEX_SHADER_BIT,           VK_PIPELINE_STAGE_VERTEX_SHADER_BIT },
    { RPIPELINE_STAGE_FRAGMENT_SHADER_BIT,         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT },
    { RPIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,    VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT },
    { RPIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,     VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT },
    { RPIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT },
    { RPIPELINE_STAGE_COMPUTE_SHADER_BIT,          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT },
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
    { RACCESS_INDIRECT_COMMAND_READ_BIT,          VK_ACCESS_INDIRECT_COMMAND_READ_BIT },
    { RACCESS_INDEX_READ_BIT,                     VK_ACCESS_INDEX_READ_BIT },
    { RACCESS_VERTEX_ATTRIBUTE_READ_BIT,          VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT },
    { RACCESS_SHADER_READ_BIT,                    VK_ACCESS_SHADER_READ_BIT },
    { RACCESS_SHADER_WRITE_BIT,                   VK_ACCESS_SHADER_WRITE_BIT },
    { RACCESS_COLOR_ATTACHMENT_READ_BIT,          VK_ACCESS_COLOR_ATTACHMENT_READ_BIT },
    { RACCESS_COLOR_ATTACHMENT_WRITE_BIT,         VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT },
    { RACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,  VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT},
    { RACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT},
    { RACCESS_TRANSFER_READ_BIT,                  VK_ACCESS_TRANSFER_READ_BIT },
    { RACCESS_TRANSFER_WRITE_BIT,                 VK_ACCESS_TRANSFER_WRITE_BIT },
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
    { RSHADER_TYPE_COMPUTE,  VK_SHADER_STAGE_COMPUTE_BIT },
    { RSHADER_TYPE_VERTEX,   VK_SHADER_STAGE_VERTEX_BIT },
    { RSHADER_TYPE_FRAGMENT, VK_SHADER_STAGE_FRAGMENT_BIT },
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
    { RBINDING_TYPE_STORAGE_IMAGE,          VK_DESCRIPTOR_TYPE_STORAGE_IMAGE },
    { RBINDING_TYPE_UNIFORM_BUFFER,         VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER },
    { RBINDING_TYPE_STORAGE_BUFFER,         VK_DESCRIPTOR_TYPE_STORAGE_BUFFER },
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
    { RBUFFER_USAGE_UNIFORM_BIT,      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT },
    { RBUFFER_USAGE_STORAGE_BIT,      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT },
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
    { RIMAGE_USAGE_TRANSFER_SRC_BIT,             VK_IMAGE_USAGE_TRANSFER_SRC_BIT },
    { RIMAGE_USAGE_TRANSFER_DST_BIT,             VK_IMAGE_USAGE_TRANSFER_DST_BIT },
    { RIMAGE_USAGE_SAMPLED_BIT,                  VK_IMAGE_USAGE_SAMPLED_BIT },
    { RIMAGE_USAGE_STORAGE_BIT,                  VK_IMAGE_USAGE_STORAGE_BIT },
    { RIMAGE_USAGE_COLOR_ATTACHMENT_BIT,         VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT },
    { RIMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT },
    { RIMAGE_USAGE_TRANSIENT_BIT,                VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT },
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
    VkImageViewType vkViewType;
} imageTypeTable[] = {
    { RIMAGE_TYPE_2D,    VK_IMAGE_TYPE_2D, VK_IMAGE_VIEW_TYPE_2D },
    { RIMAGE_TYPE_CUBE,  VK_IMAGE_TYPE_2D, VK_IMAGE_VIEW_TYPE_CUBE },
};
// clang-format on

void cast_image_type_vk(const RImageType& inType, VkImageType& outType)
{
    outType = imageTypeTable[(int)inType].vkType;
}

void cast_image_view_type_vk(const RImageType& inType, VkImageViewType& outType)
{
    outType = imageTypeTable[(int)inType].vkViewType;
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

// clang-format off
struct
{
    RPrimitiveTopology topo;
    VkPrimitiveTopology vkTopo;
} primitiveTopologyTable[] = {
    { RPRIMITIVE_TOPOLOGY_TRIANGLE_LIST,   VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST },
    { RPRIMITIVE_TOPOLOGY_POINT_LIST,      VK_PRIMITIVE_TOPOLOGY_POINT_LIST },
    { RPRIMITIVE_TOPOLOGY_LINE_LIST,       VK_PRIMITIVE_TOPOLOGY_LINE_LIST },
};
// clang-format on

void cast_primitive_topology_vk(const RPrimitiveTopology& inTopo, VkPrimitiveTopology& outTopo)
{
    outTopo = primitiveTopologyTable[(int)inTopo].vkTopo;
}

static_assert(RSAMPLE_COUNT_1_BIT == VK_SAMPLE_COUNT_1_BIT);
static_assert(RSAMPLE_COUNT_2_BIT == VK_SAMPLE_COUNT_2_BIT);
static_assert(RSAMPLE_COUNT_4_BIT == VK_SAMPLE_COUNT_4_BIT);
static_assert(RSAMPLE_COUNT_8_BIT == VK_SAMPLE_COUNT_8_BIT);
static_assert(RSAMPLE_COUNT_16_BIT == VK_SAMPLE_COUNT_16_BIT);
static_assert(RSAMPLE_COUNT_32_BIT == VK_SAMPLE_COUNT_32_BIT);
static_assert(RSAMPLE_COUNT_64_BIT == VK_SAMPLE_COUNT_64_BIT);

void cast_sample_count_vk(const RSampleCountBit& inBit, VkSampleCountFlagBits& outBit)
{
    outBit = (VkSampleCountFlagBits)inBit;
}

void cast_sample_count_from_vk(const VkSampleCountFlagBits& inBit, RSampleCountBit& outBit)
{
    outBit = (RSampleCountBit)inBit;
}

// clang-format off
struct
{
    RPolygonMode mode;
    VkPolygonMode vkMode;
} polygonModeTable[] = {
    { RPOLYGON_MODE_FILL,  VK_POLYGON_MODE_FILL },
    { RPOLYGON_MODE_LINE,  VK_POLYGON_MODE_LINE },
    { RPOLYGON_MODE_POINT, VK_POLYGON_MODE_POINT },
};
// clang-format on

void cast_polygon_mode_vk(const RPolygonMode& inMode, VkPolygonMode& outMode)
{
    outMode = polygonModeTable[(int)inMode].vkMode;
}

// clang-format off
struct
{
    RCullMode mode;
    VkCullModeFlags vkMode;
} cullModeTable[] = {
    { RCULL_MODE_NONE,  VK_CULL_MODE_NONE },
    { RCULL_MODE_FRONT, VK_CULL_MODE_FRONT_BIT },
    { RCULL_MODE_BACK,  VK_CULL_MODE_BACK_BIT },
};
// clang-format on

void cast_cull_mode_vk(const RCullMode& inMode, VkCullModeFlags& outMode)
{
    outMode = cullModeTable[(int)inMode].vkMode;
}

// clang-format off
struct
{
    RCompareOp op;
    VkCompareOp vkOp;
} compareOpTable[] = {
    { RCOMPARE_OP_NEVER,            VK_COMPARE_OP_NEVER },
    { RCOMPARE_OP_LESS,             VK_COMPARE_OP_LESS },
    { RCOMPARE_OP_EQUAL,            VK_COMPARE_OP_EQUAL },
    { RCOMPARE_OP_LESS_OR_EQUAL,    VK_COMPARE_OP_LESS_OR_EQUAL },
    { RCOMPARE_OP_GREATER,          VK_COMPARE_OP_GREATER },
    { RCOMPARE_OP_NOT_EQUAL,        VK_COMPARE_OP_NOT_EQUAL },
    { RCOMPARE_OP_GREATER_OR_EQUAL, VK_COMPARE_OP_GREATER_OR_EQUAL },
    { RCOMPARE_OP_ALWAYS,           VK_COMPARE_OP_ALWAYS },
};
// clang-format on

void cast_compare_op_vk(const RCompareOp& inOp, VkCompareOp& outOp)
{
    outOp = compareOpTable[(int)inOp].vkOp;
}

// clang-format off
struct
{
    RBlendFactor factor;
    VkBlendFactor vkFactor;
} blendFactorTable[] = {
	{ RBLEND_FACTOR_ZERO,                VK_BLEND_FACTOR_ZERO,                },
	{ RBLEND_FACTOR_ONE,                 VK_BLEND_FACTOR_ONE,                 },
	{ RBLEND_FACTOR_SRC_ALPHA,           VK_BLEND_FACTOR_SRC_ALPHA,           },
	{ RBLEND_FACTOR_DST_ALPHA,           VK_BLEND_FACTOR_DST_ALPHA,           },
	{ RBLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, },
	{ RBLEND_FACTOR_ONE_MINUS_DST_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA, },
};
// clang-format on

void cast_blend_factor_vk(const RBlendFactor& inFactor, VkBlendFactor& outFactor)
{
    outFactor = blendFactorTable[(int)inFactor].vkFactor;
}

// clang-format off
struct
{
    RBlendOp op;
    VkBlendOp vkOp;
} blendOpTable[] = {
	{ RBLEND_OP_ADD,              VK_BLEND_OP_ADD,              },
	{ RBLEND_OP_SUBTRACT,         VK_BLEND_OP_SUBTRACT,         },
	{ RBLEND_OP_REVERSE_SUBTRACT, VK_BLEND_OP_REVERSE_SUBTRACT, },
	{ RBLEND_OP_MIN,              VK_BLEND_OP_MIN,              },
	{ RBLEND_OP_MAX,              VK_BLEND_OP_MAX,              },
};
// clang-format on

void cast_blend_op_vk(const RBlendOp& inOp, VkBlendOp& outOp)
{
    outOp = blendOpTable[(int)inOp].vkOp;
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