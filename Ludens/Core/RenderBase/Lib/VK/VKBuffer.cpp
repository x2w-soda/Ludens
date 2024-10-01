#include <iostream>
#include "Core/Header/Include/Error.h"
#include "Core/RenderBase/Include/VK/VKInfo.h"
#include "Core/RenderBase/Include/VK/VKBuffer.h"
#include "Core/RenderBase/Include/VK/VKImage.h"
#include "Core/RenderBase/Include/VK/VKContext.h"
#include "Core/RenderBase/Include/VK/VKCommand.h"

namespace LD
{

VKBuffer::VKBuffer()
{
}

VKBuffer::~VKBuffer()
{
    LD_DEBUG_ASSERT(mHandle == VK_NULL_HANDLE);
}

void VKBuffer::Startup(VKDevice& device, const VKBufferInfo& info)
{
    mDevice = &device;
    mInfo = info;

    VkDevice logical = device.GetHandle();

    VK_ASSERT(vkCreateBuffer(logical, &mInfo.CreateInfo, nullptr, &mHandle));

    vkGetBufferMemoryRequirements(logical, mHandle, &mBufferMemoryRequirements);
    const VkMemoryRequirements& req = mBufferMemoryRequirements;

    uint32_t memoryTypeIndex;
    bool result = device.GetMemoryType(req.memoryTypeBits, mInfo.MemoryProperties, &memoryTypeIndex);
    LD_DEBUG_ASSERT(result && "VKBuffer::Startup device GetMemoryType failed");

    VKMemorySpec memSpec;
    memSpec.MemoryType = memoryTypeIndex;
    memSpec.Size = req.size;
    mBufferMemory.Allocate(device, memSpec);

    VK_ASSERT(vkBindBufferMemory(logical, mHandle, mBufferMemory.GetHandle(), 0));
}

void VKBuffer::Cleanup()
{
    LD_DEBUG_ASSERT(mDevice);

    VkDevice logical = mDevice->GetHandle();

    vkDestroyBuffer(logical, mHandle, nullptr);

    // the memory should be freed after the buffer is out of scope
    mBufferMemory.Free(*mDevice);

    mHandle = VK_NULL_HANDLE;
}

void VKBuffer::StageData(u32 dataSize, const void* data, VKCommandPool& transferPool, VkQueue transferQueue)
{
    LD_DEBUG_ASSERT(mInfo.MemoryProperties & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    LD_DEBUG_ASSERT(mInfo.CreateInfo.usage & VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    LD_DEBUG_ASSERT(mInfo.CreateInfo.size == dataSize && "only supports full buffer staging");
    LD_DEBUG_ASSERT(mDevice && data);

    VKBuffer stagingBuffer;
    VKBufferInfo bufferConfig;
    bufferConfig.MemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    bufferConfig.CreateInfo = VKInfo::BufferCreate(dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    stagingBuffer.Startup(*mDevice, bufferConfig);

    void* dataMap = stagingBuffer.Map();
    memcpy(dataMap, data, dataSize);
    stagingBuffer.Unmap();

    stagingBuffer.CopyToBuffer(*this, transferPool, transferQueue);
    stagingBuffer.Cleanup();
}

void VKBuffer::CopyToBuffer(VKBuffer& dstBuffer, VKCommandPool& transferPool, VkQueue transferQueue)
{
    LD_DEBUG_ASSERT(mDevice);
    LD_DEBUG_ASSERT(mInfo.CreateInfo.size == dstBuffer.mInfo.CreateInfo.size &&
                    "only supports full buffer copy between equal sized buffers");

    VKCommandBuffer command;
    command.AllocatePrimary(*mDevice, transferPool, 1);

    VkCommandBuffer commandBuffer = command.GetHandle();
    command.BeginRecord(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    {
        VkBufferCopy region{};
        region.srcOffset = 0;
        region.dstOffset = 0;
        region.size = mInfo.CreateInfo.size;
        vkCmdCopyBuffer(commandBuffer, mHandle, dstBuffer.mHandle, 1, &region);
    }
    command.EndRecord();
    command.Submit(transferQueue);

    VK_ASSERT(vkQueueWaitIdle(transferQueue));

    command.Free(*mDevice);
}

void VKBuffer::CopyToImage(VKImage& dstImage, VkImageSubresourceRange range, const Vector<VkBufferImageCopy>& regions,
                           VKCommandPool& transferPool, VkQueue transferQueue)
{
    VKCommandBuffer command;
    command.AllocatePrimary(*mDevice, transferPool, 1);

    VkCommandBuffer commandBuffer = command.GetHandle();
    u32 imageWidth = dstImage.GetWidth();
    u32 imageHeight = dstImage.GetHeight();

    command.BeginRecord(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    {
        // image layout transition via image memory barriers
        // prepare to copy from staging buffer to image
        command.CmdImageLayoutTransition(dstImage, range, VK_IMAGE_LAYOUT_UNDEFINED,
                                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        // perform copy from staging buffer to image
        vkCmdCopyBufferToImage(commandBuffer, this->GetHandle(), dstImage.GetHandle(),
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, regions.Size(), regions.Data());

        // transition for device local image memory to be shader read only optimal
        command.CmdImageLayoutTransition(dstImage, range, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
    command.EndRecord();
    command.Submit(transferQueue);

    VK_ASSERT(vkQueueWaitIdle(transferQueue));

    command.Free(*mDevice);
}

void* VKBuffer::Map()
{
    return mBufferMemory.Map(*mDevice);
}

void VKBuffer::Unmap()
{
    mBufferMemory.Unmap(*mDevice);
}

} // namespace LD