#include <iostream>
#include "Core/Header/Include/Types.h"
#include "Core/Header/Include/Error.h"
#include "Core/RenderBase/Include/VK/VKInfo.h"
#include "Core/RenderBase/Include/VK/VKImage.h"
#include "Core/RenderBase/Include/VK/VKBuffer.h"
#include "Core/RenderBase/Include/VK/VKDevice.h"
#include "Core/RenderBase/Include/VK/VKContext.h"

namespace LD
{

void VKImageView::Startup(const VKDevice& device, const VkImageViewCreateInfo& info)
{
    mDevice = device.GetHandle();

    VK_ASSERT(vkCreateImageView(mDevice, &info, nullptr, &mView));

    mHasStartup = true;
}

void VKImageView::Cleanup()
{
    mHasStartup = false;

    vkDestroyImageView(mDevice, mView, nullptr);
}

void VKImage::Startup(VKDevice& device, const VKImageInfo& info)
{
    mInfo = info;
    mDevice = &device;
    VkDevice logical = device.GetHandle();

    VK_ASSERT(vkCreateImage(logical, &mInfo.CreateInfo, nullptr, &mImage));

    VkMemoryRequirements memReq;
    vkGetImageMemoryRequirements(logical, mImage, &memReq);

    u32 memoryTypeIndex;
    if (!device.GetMemoryType(memReq.memoryTypeBits, mInfo.MemoryProperties, &memoryTypeIndex))
    {
        std::cout << "device GetMemoryType failed" << std::endl;
        return;
    }

    VKMemorySpec memSpec;
    memSpec.MemoryType = memoryTypeIndex;
    memSpec.Size = memReq.size;
    mImageMemory.Allocate(device, memSpec);

    VK_ASSERT(vkBindImageMemory(logical, mImage, mImageMemory.GetHandle(), 0));
}

void VKImage::Cleanup()
{
    LD_DEBUG_ASSERT(mDevice && mImage != VK_NULL_HANDLE);

    VkDevice logical = mDevice->GetHandle();

    vkDestroyImage(logical, mImage, nullptr);

    // the memory should be freed after the image is out of scope
    mImageMemory.Free(*mDevice);
}

void VKImage::StageData(u32 layerCount, u32 dataSize, const void** data, VKCommandPool& transferPool,
                        VkQueue transferQueue)
{
    LD_DEBUG_ASSERT(mInfo.MemoryProperties & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    LD_DEBUG_ASSERT(mInfo.CreateInfo.usage & VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    LD_DEBUG_ASSERT(mInfo.CreateInfo.arrayLayers == layerCount && "layer count mismatch");
    LD_DEBUG_ASSERT(mDevice && data);

    VKBuffer stagingBuffer;
    VKBufferInfo bufferConfig;
    bufferConfig.MemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    bufferConfig.CreateInfo = VKInfo::BufferCreate(layerCount * dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    stagingBuffer.Startup(*mDevice, bufferConfig);

    void* dataMap = stagingBuffer.Map();
    for (u32 layer = 0; layer < layerCount; layer++)
    {
        u8* dst = (u8*)dataMap + dataSize * layer;
        memcpy((void*)dst, data[layer], dataSize);
    }
    stagingBuffer.Unmap();

    // image copy regions
    Vector<VkBufferImageCopy> regions;
    for (u32 layer = 0; layer < layerCount; layer++)
    {
        VkBufferImageCopy region{};
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = layer;
        region.imageSubresource.layerCount = 1;
        region.imageExtent = mInfo.CreateInfo.extent;
        region.bufferOffset = dataSize * layer;
        regions.PushBack(region);
    }

    // image layout transition range
    VkImageSubresourceRange range{};
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseArrayLayer = 0;
    range.layerCount = mInfo.CreateInfo.arrayLayers;
    range.baseMipLevel = 0;
    range.levelCount = 1;

    stagingBuffer.CopyToImage(*this, range, regions, transferPool, transferQueue);
    stagingBuffer.Cleanup();
}

void VKSampler::Startup(const VKDevice& device, const VkSamplerCreateInfo& info)
{
    mDevice = device.GetHandle();

    VK_ASSERT(vkCreateSampler(mDevice, &info, nullptr, &mSampler));
}

void VKSampler::Cleanup()
{
    LD_DEBUG_ASSERT(mDevice != VK_NULL_HANDLE && mSampler != VK_NULL_HANDLE);

    vkDestroySampler(mDevice, mSampler, nullptr);
}

} // namespace LD