#pragma once

#include <vulkan/vulkan.h>
#include "Core/Header/Include/Types.h"
#include "Core/RenderBase/Include/VK/VKContext.h"

namespace LD
{

class VKDevice;
class VKImage;

class VKCommandPool
{
public:
    VKCommandPool() = default;
    VKCommandPool(const VKCommandPool&) = delete;
    ~VKCommandPool() = default;

    VKCommandPool& operator=(const VKCommandPool&) = delete;

    void Startup(VKDevice& device, const VkCommandPoolCreateInfo& commandPoolCI);
    void Cleanup();

    inline u32 GetQueueFamilyIndex() const
    {
        return mQueueFamilyIndex;
    }
    inline VkCommandPool GetHandle()
    {
        return mHandle;
    }

private:
    VkCommandPool mHandle = VK_NULL_HANDLE;
    VkDevice mLogical = VK_NULL_HANDLE;
    u32 mQueueFamilyIndex;
};

class VKCommandBuffer
{
public:
    VKCommandBuffer() = default;
    VKCommandBuffer(const VKCommandBuffer&) = delete;
    ~VKCommandBuffer() = default;

    VKCommandBuffer& operator=(const VKCommandBuffer&) = delete;

    void AllocatePrimary(VKDevice& device, VKCommandPool& pool, u32 count);
    void AllocateSecondary(VKDevice& device, VKCommandPool& pool, u32 count);
    void Free(VKDevice& device);

    // below API are only available between Allocate*() and Free()
    inline VkCommandBuffer GetHandle()
    {
        return mHandle;
    }

    void BeginRecord(VkCommandBufferUsageFlags usage);
    void EndRecord();
    void Submit(VkQueue queue);
    void Reset(VkCommandBufferResetFlags flags);

    // below API are only available between BeginRecord() and EndRecord()
    void CmdBeginRenderPass(const VkRenderPassBeginInfo& renderPassBI);
    void CmdEndRenderPass();

    void CmdBindVertexBuffers(u32 firstBinding, u32 bindingCount, const VkBuffer* buffers, const VkDeviceSize* offsets);
    void CmdBindVertexBuffer(u32 binding, VkBuffer buffer, VkDeviceSize offset);
    void CmdBindIndexBuffer(VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType);
    void CmdBindPipeline(VkPipelineBindPoint bindPoint, VkPipeline pipeline);
    void CmdBindGraphicsPipeline(VkPipeline pipeline);
    void CmdSetViewport(VkViewport viewport);
    void CmdSetScissor(VkRect2D scissor);

    void CmdDrawVertex(u32 vertexCount, u32 instanceCount);
    void CmdDrawIndexed(u32 indexCount, u32 instanceCount, u32 indexStart);

    void CmdImageLayoutTransition(VKImage& image, VkImageSubresourceRange range, VkImageLayout oldLayout,
                                  VkImageLayout newLayout);

private:
    void Allocate(VKDevice& device, VKCommandPool& pool, u32 count, VkCommandBufferLevel level);

    // helpers for recording commands related to image memory barriers
    typedef struct
    {
        static void ImageLayoutTransition(VkImageMemoryBarrier* barrier, VkPipelineStageFlags* srcStage,
                                          VkPipelineStageFlags* dstStage);
    } ImageMemoryBarrier;

    VkCommandBuffer mHandle = VK_NULL_HANDLE;
    VkCommandPool mPool = VK_NULL_HANDLE;
    VkDevice mLogical = VK_NULL_HANDLE;
    bool mIsAllocated = false;
    bool mIsRecording = false;
};

inline void VKCommandBuffer::BeginRecord(VkCommandBufferUsageFlags usage)
{
    LD_DEBUG_ASSERT(mIsAllocated);

    VkCommandBufferBeginInfo bufferBI{};
    bufferBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bufferBI.pInheritanceInfo = nullptr;
    bufferBI.flags = usage;

    VK_ASSERT(vkBeginCommandBuffer(mHandle, &bufferBI));
    mIsRecording = true;
}

inline void VKCommandBuffer::EndRecord()
{
    LD_DEBUG_ASSERT(mIsAllocated);
    LD_DEBUG_ASSERT(mIsRecording);

    VK_ASSERT(vkEndCommandBuffer(mHandle));
    mIsRecording = false;
}

inline void VKCommandBuffer::Submit(VkQueue queue)
{
    LD_DEBUG_ASSERT(mIsAllocated);
    LD_DEBUG_ASSERT(!mIsRecording);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &mHandle;

    VK_ASSERT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
}

inline void VKCommandBuffer::Reset(VkCommandBufferResetFlags flags)
{
    VK_ASSERT(vkResetCommandBuffer(mHandle, flags));
}

inline void VKCommandBuffer::CmdBeginRenderPass(const VkRenderPassBeginInfo& renderPassBI)
{
    vkCmdBeginRenderPass(mHandle, &renderPassBI, VK_SUBPASS_CONTENTS_INLINE);
}

inline void VKCommandBuffer::CmdEndRenderPass()
{
    vkCmdEndRenderPass(mHandle);
}

//
// Binding Commands
//

inline void VKCommandBuffer::CmdBindVertexBuffers(u32 firstBinding, u32 bindingCount, const VkBuffer* buffers,
                                                  const VkDeviceSize* offsets)
{
    vkCmdBindVertexBuffers(mHandle, firstBinding, bindingCount, buffers, offsets);
}

inline void VKCommandBuffer::CmdBindVertexBuffer(u32 binding, VkBuffer buffer, VkDeviceSize offset)
{
    vkCmdBindVertexBuffers(mHandle, binding, 1, &buffer, &offset);
}

inline void VKCommandBuffer::CmdBindIndexBuffer(VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType)
{
    vkCmdBindIndexBuffer(mHandle, buffer, offset, indexType);
}

inline void VKCommandBuffer::CmdBindPipeline(VkPipelineBindPoint bindPoint, VkPipeline pipeline)
{
    vkCmdBindPipeline(mHandle, bindPoint, pipeline);
}

inline void VKCommandBuffer::CmdBindGraphicsPipeline(VkPipeline pipeline)
{
    vkCmdBindPipeline(mHandle, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

inline void VKCommandBuffer::CmdSetViewport(VkViewport viewport)
{
    vkCmdSetViewport(mHandle, 0, 1, &viewport);
}

inline void VKCommandBuffer::CmdSetScissor(VkRect2D scissor)
{
    vkCmdSetScissor(mHandle, 0, 1, &scissor);
}

//
// Draw Calls
//

inline void VKCommandBuffer::CmdDrawVertex(u32 vertexCount, u32 instanceCount)
{
    vkCmdDraw(mHandle, vertexCount, instanceCount, 0, 0);
}

inline void VKCommandBuffer::CmdDrawIndexed(u32 indexCount, u32 instanceCount, u32 indexStart)
{
    vkCmdDrawIndexed(mHandle, indexCount, instanceCount, indexStart, 0, 0);
}

} // namespace LD