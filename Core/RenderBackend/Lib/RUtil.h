#pragma once

#include <Ludens/RenderBackend/RBackend.h>
#include <cstdint>
#include <string>
#include <vulkan/vulkan.h> // hide from user

namespace LD {
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

void cast_clear_color_value_vk(const RClearColorValue& inValue, VkClearColorValue& outValue);
void cast_format_vk(const RFormat& inFormat, VkFormat& outFormat);
void cast_glsl_type_vk(const RGLSLType& inType, VkFormat& outFormat);
void cast_image_layout_vk(const RImageLayout& inLayout, VkImageLayout& outLayout);
void cast_attachment_load_op_vk(const RAttachmentLoadOp& inOp, VkAttachmentLoadOp& outOp);
void cast_attachment_store_op_vk(const RAttachmentStoreOp& inOp, VkAttachmentStoreOp& outOp);
void cast_pass_color_attachment_vk(const RPassColorAttachment& inAttachment, VkAttachmentDescription& outDesc);
void cast_pass_depth_stencil_attachment_vk(const RPassDepthStencilAttachment& inAttachment, VkAttachmentDescription& outDesc);
void cast_pipeline_stage_bits_vk(const RPipelineStageBits& inBits, VkPipelineStageFlags& outBits);
void cast_access_bits_vk(const RAccessBits& inBits, VkAccessFlags& outBits);
void cast_pass_dependency_vk(const RPassDependency& inDep, const uint32_t inSrcSubpass, const uint32_t inDstSubpass, VkSubpassDependency& outDep);
void cast_shader_type_vk(const RShaderType& inType, VkShaderStageFlagBits& outType);
void cast_binding_type_vk(const RBindingType& inType, VkDescriptorType& outType);
void cast_set_layout_binding_vk(const RSetBindingInfo& inBinding, VkDescriptorSetLayoutBinding& outBinding);
void cast_vertex_attribute_vk(const RVertexAttribute& inAttr, uint32_t inLocation, VkVertexInputAttributeDescription& outAttr);
void cast_vertex_binding_vk(const RVertexBinding& inBinding, uint32_t inIndex, VkVertexInputBindingDescription& outBinding);
void cast_buffer_type_vk(const RBufferType& inType, VkBufferUsageFlags& outFlags);

void print_vk_queue_flags(const VkQueueFlags& inFlags, std::string& out);
void print_vk_present_mode(const VkPresentModeKHR& inMode, std::string& out);

} // namespace RUtil
} // namespace LD