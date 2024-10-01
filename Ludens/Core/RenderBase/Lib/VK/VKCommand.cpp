#include "Core/Header/Include/Error.h"
#include "Core/RenderBase/Include/VK/VKContext.h"
#include "Core/RenderBase/Include/VK/VKFormat.h"
#include "Core/RenderBase/Include/VK/VKCommand.h"
#include "Core/RenderBase/Include/VK/VKDevice.h"

namespace LD
{

void VKCommandPool::Startup(VKDevice& device, const VkCommandPoolCreateInfo& commandPoolCI)
{
    mLogical = device.GetHandle();
    mQueueFamilyIndex = commandPoolCI.queueFamilyIndex;

    VK_ASSERT(vkCreateCommandPool(mLogical, &commandPoolCI, nullptr, &mHandle))
}

void VKCommandPool::Cleanup()
{
    vkDestroyCommandPool(mLogical, mHandle, nullptr);
}

void VKCommandBuffer::Allocate(VKDevice& device, VKCommandPool& pool, u32 count, VkCommandBufferLevel level)
{
    mLogical = device.GetHandle();
    mPool = pool.GetHandle();

    VkCommandBufferAllocateInfo bufferAI{};
    bufferAI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    bufferAI.level = level;
    bufferAI.commandPool = mPool;
    bufferAI.commandBufferCount = count;

    VK_ASSERT(vkAllocateCommandBuffers(mLogical, &bufferAI, &mHandle));

    mIsAllocated = true;
}

void VKCommandBuffer::AllocatePrimary(VKDevice& device, VKCommandPool& pool, u32 count)
{
    Allocate(device, pool, count, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
}

void VKCommandBuffer::AllocateSecondary(VKDevice& device, VKCommandPool& pool, u32 count)
{
    Allocate(device, pool, count, VK_COMMAND_BUFFER_LEVEL_SECONDARY);
}

void VKCommandBuffer::Free(VKDevice& device)
{
    mIsAllocated = false;

    vkFreeCommandBuffers(mLogical, mPool, 1, &mHandle);

    mPool = VK_NULL_HANDLE;
    mLogical = VK_NULL_HANDLE;
    mHandle = VK_NULL_HANDLE;
}

void VKCommandBuffer::CmdImageLayoutTransition(VKImage& image, VkImageSubresourceRange range, VkImageLayout oldLayout,
                                               VkImageLayout newLayout)
{
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image.GetHandle();
    barrier.subresourceRange = range;
    barrier.srcAccessMask = 0; // decided by helper
    barrier.dstAccessMask = 0; // decided by helper

    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (VKFormat::HasStencilComponent(image.GetFormat()))
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    else
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    VkPipelineStageFlags srcStageMasks;
    VkPipelineStageFlags dstStageMasks;
    ImageMemoryBarrier::ImageLayoutTransition(&barrier, &srcStageMasks, &dstStageMasks);

    vkCmdPipelineBarrier(mHandle, srcStageMasks, dstStageMasks, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

// READS: the image memory barrier's old and new layout
// WRITES: the src and dst access flags for the barrier
// WRITES: the src and dst pipeline stage for the transition
// the compatability between access flags and pipeline stages are listed in this table:
// https://registry.khronos.org/vulkan/specs/1.3-extensions/html/chap7.html#synchronization-access-types-supported
void VKCommandBuffer::ImageMemoryBarrier::ImageLayoutTransition(VkImageMemoryBarrier* barrier,
                                                                VkPipelineStageFlags* srcStage,
                                                                VkPipelineStageFlags* dstStage)
{
    LD_DEBUG_ASSERT(barrier && srcStage && dstStage);

    VkImageLayout oldLayout = barrier->oldLayout;
    VkImageLayout newLayout = barrier->newLayout;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier->srcAccessMask = 0;
        barrier->dstAccessMask =
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        *srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        *dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        return;
    }

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier->srcAccessMask = 0;
        barrier->dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        *srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        *dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        return;
    }

    if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier->srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier->dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        *srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        *dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        return;
    }

    LD_DEBUG_ASSERT(false && "Unsupported layout transition");
}

} // namespace LD