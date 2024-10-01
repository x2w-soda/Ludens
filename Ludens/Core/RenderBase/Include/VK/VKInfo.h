#pragma once

#include <cstdio>
#include <vulkan/vulkan_core.h>
#include "Core/Header/Include/Types.h"
#include "Core/Header/Include/Error.h"
#include "Core/RenderBase/Include/VK/VKFormat.h"

namespace LD {
namespace VKInfo {

///
/// MISC
///

inline VkRect2D Rect2D(VkExtent2D extent)
{
    VkRect2D rect{};
    rect.offset.x = 0;
    rect.offset.y = 0;
    rect.extent = extent;
    return rect;
}

inline VkViewport Viewport(VkRect2D rect)
{
    VkViewport viewport{};
    viewport.x = rect.offset.x;
    viewport.y = rect.offset.y;
    viewport.width = rect.extent.width;
    viewport.height = rect.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    return viewport;
}

inline VkViewport Viewport(VkExtent2D extent)
{
    VkViewport viewport{};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = (float)extent.width;
    viewport.height = (float)extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    return viewport;
}

///
/// BUFFERS AND IMAGES
///

inline VkBufferCreateInfo BufferCreate(u32 size, VkBufferUsageFlags usage,
                                       VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE)
{
    VkBufferCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.size = size;
    info.usage = usage;
    info.sharingMode = sharingMode;
    return info;
}

inline VkImageCreateInfo Image2DCreate(VkFormat format, VkExtent2D extent, VkImageUsageFlags usage,
                                       VkSharingMode sharingMode, VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL,
                                       VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED)
{
    VkImageCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.imageType = VK_IMAGE_TYPE_2D;
    info.format = format;
    info.extent.width = extent.width;
    info.extent.height = extent.height;
    info.extent.depth = 1;
    info.mipLevels = 1;
    info.arrayLayers = 1;
    info.samples = VK_SAMPLE_COUNT_1_BIT;
    info.tiling = tiling;
    info.initialLayout = initialLayout;
    info.usage = usage;
    info.sharingMode = sharingMode;
    info.flags = 0;
    return info;
}

inline VkImageCreateInfo Image2DCreateCubemap(VkFormat format, VkExtent2D extent, VkImageUsageFlags usage,
                                              VkSharingMode sharingMode, VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL,
                                              VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED)
{
    VkImageCreateInfo info = Image2DCreate(format, extent, usage, sharingMode, tiling, initialLayout);
    info.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT; // required flags for cubemap
    info.arrayLayers = 6;                              // required layers for cubemap
    return info;
}

inline VkImageViewCreateInfo ImageViewCreate(VkImageViewType type, VkImage image, VkFormat format,
                                             VkImageSubresourceRange range)
{
    VkImageViewCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.image = image;
    info.viewType = type;
    info.format = format;
    info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    info.subresourceRange = range;
    return info;
}

// image view creation for subresource range with single mip map level and single layer
inline VkImageViewCreateInfo ImageViewCreate(VkImageViewType type, VkImage image, VkFormat format,
                                             VkImageAspectFlags aspectFlags)
{
    VkImageSubresourceRange range{};
    range.aspectMask = aspectFlags;
    range.baseMipLevel = 0;
    range.levelCount = 1;
    range.baseArrayLayer = 0;
    range.layerCount = 1;
    return ImageViewCreate(type, image, format, range);
}

// image view creation for cubemap with single mip map level
inline VkImageViewCreateInfo ImageViewCreateCubemap(VkImage image, VkFormat format)
{
    VkImageSubresourceRange range;
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseArrayLayer = 0;
    range.layerCount = 6;
    range.baseMipLevel = 0;
    range.levelCount = 1;
    return ImageViewCreate(VK_IMAGE_VIEW_TYPE_CUBE, image, format, range);
}

inline VkSamplerCreateInfo SamplerCreate(VkFilter magFilter, VkFilter minFilter, VkSamplerAddressMode addressMode,
                                         float maxAnisotropy)
{
    VkSamplerCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.magFilter = magFilter;
    info.minFilter = minFilter;
    info.addressModeU = addressMode;
    info.addressModeV = addressMode;
    info.addressModeW = addressMode;
    info.anisotropyEnable = VK_TRUE;
    info.maxAnisotropy = maxAnisotropy;
    info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    info.unnormalizedCoordinates = VK_FALSE;
    info.compareEnable = VK_FALSE;
    info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    info.mipLodBias = 0.0f;
    info.minLod = 0.0f;
    info.maxLod = 0.0f;
    return info;
}

///
/// RENDER PASS
///

inline VkAttachmentDescription AttachmentDesc(VkFormat format, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp,
                                              VkAttachmentLoadOp stencillLoadOp, VkAttachmentStoreOp stencilStoreOp,
                                              VkImageLayout initialLayout, VkImageLayout finalLayout,
                                              VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT)
{
    VkAttachmentDescription desc{};
    desc.format = format;
    desc.samples = samples;
    desc.loadOp = loadOp;
    desc.storeOp = storeOp;
    desc.stencilLoadOp = stencillLoadOp;
    desc.stencilStoreOp = stencilStoreOp;
    desc.initialLayout = initialLayout;
    desc.finalLayout = finalLayout;
    return desc;
}

inline VkAttachmentDescription AttachmentDescColorDepth(VkFormat format, VkAttachmentLoadOp loadOp,
                                                        VkAttachmentStoreOp storeOp, VkImageLayout initialLayout,
                                                        VkImageLayout finalLayout,
                                                        VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT)
{
    LD_DEBUG_CANARY(!VKFormat::HasStencilComponent(format), [&](const char*) {
        printf("format %u has a stencil component, but the contents aren't preserved across subpasses\n", format);
    });

    return AttachmentDesc(format, loadOp, storeOp, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
                          initialLayout, finalLayout, samples);
}

inline VkAttachmentDescription AttachmentDescStencil(VkFormat format, VkAttachmentLoadOp stencillLoadOp,
                                                     VkAttachmentStoreOp stencilStoreOp, VkImageLayout initialLayout,
                                                     VkImageLayout finalLayout,
                                                     VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT)
{
    LD_DEBUG_CANARY(!VKFormat::HasDepthComponent(format), [&](const char*) {
        printf("format %u has a depth component, but the contents aren't preserved across subpasses\n", format);
    });

    return AttachmentDesc(format, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, stencillLoadOp,
                          stencilStoreOp, initialLayout, finalLayout, samples);
}

inline VkAttachmentReference AttachmentRef(u32 attachment, VkImageLayout layout)
{
    VkAttachmentReference ref{};
    ref.attachment = attachment;
    ref.layout = layout;
    return ref;
}

///
/// SHADERS AND PIPELINES
///

inline VkVertexInputAttributeDescription VertexInputAttributeDesc(u32 binding, u32 location, VkFormat format,
                                                                  u32 offset)
{
    VkVertexInputAttributeDescription desc{};
    desc.binding = binding;
    desc.location = location;
    desc.format = format;
    desc.offset = offset;
    return desc;
}

inline VkVertexInputBindingDescription VertexInputBindingDesc(u32 binding, u32 stride, VkVertexInputRate inputRate)
{
    VkVertexInputBindingDescription desc{};
    desc.binding = binding;
    desc.stride = stride;
    desc.inputRate = inputRate;
    return desc;
}

inline VkVertexInputBindingDescription VertexInputBindingDescVertex(u32 binding, u32 stride)
{
    return VKInfo::VertexInputBindingDesc(binding, stride, VK_VERTEX_INPUT_RATE_VERTEX);
}

inline VkVertexInputBindingDescription VertexInputBindingDescInstance(u32 binding, u32 stride)
{
    return VKInfo::VertexInputBindingDesc(binding, stride, VK_VERTEX_INPUT_RATE_INSTANCE);
}

inline VkPipelineShaderStageCreateInfo PipelineShaderStageCreate(VkShaderStageFlagBits shaderStage,
                                                                 VkShaderModule shaderModule,
                                                                 const char* entryName = "main")
{
    VkPipelineShaderStageCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info.stage = shaderStage;
    info.module = shaderModule;
    info.pName = entryName;
    return info;
}

inline VkPipelineShaderStageCreateInfo PipelineShaderStageCreateVertex(VkShaderModule shaderModule,
                                                                       const char* entryName = "main")
{
    return VKInfo::PipelineShaderStageCreate(VK_SHADER_STAGE_VERTEX_BIT, shaderModule, entryName);
}

inline VkPipelineShaderStageCreateInfo PipelineShaderStageCreateFragment(VkShaderModule shaderModule,
                                                                         const char* entryName = "main")
{
    return VKInfo::PipelineShaderStageCreate(VK_SHADER_STAGE_FRAGMENT_BIT, shaderModule, entryName);
}

inline VkPipelineVertexInputStateCreateInfo PipelineVertexInputStateCreate(
    u32 bindingCount, const VkVertexInputBindingDescription* bindings, u32 attributeCount,
    const VkVertexInputAttributeDescription* attributes)
{
    VkPipelineVertexInputStateCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    info.vertexBindingDescriptionCount = bindingCount;
    info.pVertexBindingDescriptions = bindings;
    info.vertexAttributeDescriptionCount = attributeCount;
    info.pVertexAttributeDescriptions = attributes;
    return info;
}

inline VkPipelineInputAssemblyStateCreateInfo PipelineInputAssemblyStateCreate(VkPrimitiveTopology topology,
                                                                               VkBool32 primitiveRestartEn)
{
    VkPipelineInputAssemblyStateCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    info.topology = topology;
    info.primitiveRestartEnable = primitiveRestartEn;
    return info;
}

inline VkPipelineViewportStateCreateInfo PipelineViewportStateCreate(u32 viewportCount, const VkViewport* viewports,
                                                                     u32 scissorCount, const VkRect2D* scissors)
{
    VkPipelineViewportStateCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    info.viewportCount = viewportCount;
    info.pViewports = viewports;
    info.scissorCount = scissorCount;
    info.pScissors = scissors;
    return info;
}

///
/// DESCRIPTORS
///

inline VkDescriptorPoolSize DescriptorPoolSize(VkDescriptorType type, u32 descriptorCount)
{
    VkDescriptorPoolSize size{};
    size.type = type;
    size.descriptorCount = descriptorCount;
    return size;
}

inline VkDescriptorPoolCreateInfo DescriptorPoolCreate(u32 poolSizeCount, const VkDescriptorPoolSize* poolSizes,
                                                       u32 maxSets, VkDescriptorPoolCreateFlags flags)
{
    VkDescriptorPoolCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    info.poolSizeCount = poolSizeCount;
    info.pPoolSizes = poolSizes;
    info.maxSets = maxSets;
    info.flags = flags;
    return info;
}

inline VkDescriptorSetLayoutBinding DescriptorSetLayoutBinding(u32 binding, VkDescriptorType type, u32 count,
                                                               VkShaderStageFlags stageFlags)
{
    VkDescriptorSetLayoutBinding info{};
    info.binding = binding;
    info.descriptorType = type;
    info.descriptorCount = count;
    info.stageFlags = stageFlags;
    info.pImmutableSamplers = nullptr;
    return info;
}

///
/// COMMAND BUFFERS
///

inline VkCommandPoolCreateInfo CommandPoolCreate(VkCommandPoolCreateFlags flags, u32 queueFamilyIndex)
{
    VkCommandPoolCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.flags = flags;
    info.queueFamilyIndex = queueFamilyIndex;
    return info;
}

inline VkRenderPassBeginInfo RenderPassBegin(VkRenderPass renderPass, VkFramebuffer frameBuffer, VkRect2D renderArea,
                                             u32 clearValueCount, const VkClearValue* clearValues)
{
    VkRenderPassBeginInfo info{};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    info.renderPass = renderPass;
    info.framebuffer = frameBuffer;
    info.renderArea = renderArea;
    info.clearValueCount = clearValueCount;
    info.pClearValues = clearValues;
    return info;
}

inline VkRenderPassBeginInfo RenderPassBegin(VkRenderPass renderPass, VkFramebuffer frameBuffer,
                                             VkExtent2D renderExtent, u32 clearValueCount,
                                             const VkClearValue* clearValues)
{
    VkRenderPassBeginInfo info{};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    info.renderPass = renderPass;
    info.framebuffer = frameBuffer;
    info.renderArea.offset = {0, 0};
    info.renderArea.extent = renderExtent;
    info.clearValueCount = clearValueCount;
    info.pClearValues = clearValues;
    return info;
}

///
/// SYNCHRONIZATION PRIMITIVES
///

inline VkFenceCreateInfo FenceCreate(VkFenceCreateFlags flags = 0)
{
    VkFenceCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    info.flags = flags;
    return info;
}

} // namespace VKInfo
} // namespace LD