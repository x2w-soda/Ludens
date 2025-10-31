#pragma once

#include <Ludens/Header/Math/Rect.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <cstdint>
#include <string>
#include <vulkan/vulkan.h> // hide from user

namespace LD {

struct RPassInfoData;
struct RPassInfo;

namespace RUtil {

inline VkViewport make_viewport(uint32_t width, uint32_t height)
{
    return VkViewport{
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)width,
        .height = (float)height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
}

inline VkRect2D make_scissor(uint32_t width, uint32_t height)
{
    return VkRect2D{
        .offset = {0, 0},
        .extent = {width, height},
    };
}

inline VkRect2D make_scissor(const Rect& rect)
{
    return VkRect2D{
        .offset = {(int32_t)rect.x, (int32_t)rect.y},
        .extent = {(uint32_t)rect.w, (uint32_t)rect.h},
    };
}

uint32_t get_format_texel_size(const RFormat& format);

void save_pass_info(const RPassInfo& inInfo, RPassInfoData& outData);
void load_pass_info(const RPassInfoData& inData, RPassInfo& outInfo);

void cast_clear_color_value_vk(const RClearColorValue& inValue, VkClearColorValue& outValue);
void cast_filter_vk(const RFilter& inFilter, VkFilter& outFilter);
void cast_filter_mipmap_mode_vk(const RFilter& inFilter, VkSamplerMipmapMode& outMipmapMode);
void cast_sampler_address_mode_vk(const RSamplerAddressMode& inMode, VkSamplerAddressMode& outMode);
void cast_format_vk(const RFormat& inFormat, VkFormat& outFormat);
void cast_format_from_vk(const VkFormat& inFormat, RFormat& outFormat);
void cast_format_image_aspect_vk(const RFormat& inFormat, VkImageAspectFlags& outFlags);
void cast_glsl_type_vk(const RGLSLType& inType, VkFormat& outFormat);
void cast_image_layout_vk(const RImageLayout& inLayout, VkImageLayout& outLayout);
void cast_attachment_load_op_vk(const RAttachmentLoadOp& inOp, VkAttachmentLoadOp& outOp);
void cast_attachment_store_op_vk(const RAttachmentStoreOp& inOp, VkAttachmentStoreOp& outOp);
void cast_pass_color_attachment_vk(const RPassColorAttachment& inAttachment, const RSampleCountBit& inSamples, VkAttachmentDescription& outDesc);
void cast_pass_color_resolve_attachment_vk(const RPassResolveAttachment& inAttachment, const RFormat& inColorFormat, VkAttachmentDescription& outDesc);
void cast_pass_depth_stencil_attachment_vk(const RPassDepthStencilAttachment& inAttachment, const RSampleCountBit& inSamples, VkAttachmentDescription& outDesc);
void cast_pipeline_stage_flags_vk(const RPipelineStageFlags& inFlags, VkPipelineStageFlags& outFlags);
void cast_access_flags_vk(const RAccessFlags& inFlags, VkAccessFlags& outFlags);
void cast_pass_dependency_vk(const RPassDependency& inDep, const uint32_t inSrcSubpass, const uint32_t inDstSubpass, VkSubpassDependency& outDep);
void cast_shader_type_vk(const RShaderType& inType, VkShaderStageFlagBits& outType);
void cast_binding_type_vk(const RBindingType& inType, VkDescriptorType& outType);
void cast_set_layout_binding_vk(const RSetBindingInfo& inBinding, VkDescriptorSetLayoutBinding& outBinding);
void cast_vertex_attribute_vk(const RVertexAttribute& inAttr, uint32_t inLocation, VkVertexInputAttributeDescription& outAttr);
void cast_vertex_binding_vk(const RVertexBinding& inBinding, uint32_t inIndex, VkVertexInputBindingDescription& outBinding);
void cast_buffer_usage_vk(const RBufferUsageFlags& inUsage, VkBufferUsageFlags& outUsage);
void cast_image_usage_vk(const RImageUsageFlags& inUsage, VkImageUsageFlags& outUsage);
void cast_image_type_vk(const RImageType& inType, VkImageType& outType);
void cast_image_view_type_vk(const RImageType& inType, VkImageViewType& outType);
void cast_index_type_vk(const RIndexType& inType, VkIndexType& outType);
void cast_primitive_topology_vk(const RPrimitiveTopology& inTopo, VkPrimitiveTopology& outTopo);
void cast_sample_count_vk(const RSampleCountBit& inBit, VkSampleCountFlagBits& outBit);
void cast_sample_count_from_vk(const VkSampleCountFlagBits& inBit, RSampleCountBit& outBit);
void cast_color_components_vk(const RColorComponentFlags& inFlags, VkColorComponentFlags& outFlags);
void cast_polygon_mode_vk(const RPolygonMode& inMode, VkPolygonMode& outMode);
void cast_cull_mode_vk(const RCullMode& inMode, VkCullModeFlags& outMode);
void cast_compare_op_vk(const RCompareOp& inOp, VkCompareOp& outOp);
void cast_blend_factor_vk(const RBlendFactor& inFactor, VkBlendFactor& outFactor);
void cast_blend_op_vk(const RBlendOp& inOp, VkBlendOp& outOp);

void print_vk_queue_flags(const VkQueueFlags& inFlags, std::string& out);
void print_vk_present_mode(const VkPresentModeKHR& inMode, std::string& out);

} // namespace RUtil
} // namespace LD