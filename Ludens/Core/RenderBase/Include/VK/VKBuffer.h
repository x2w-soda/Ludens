#pragma once

#include <vulkan/vulkan_core.h>
#include "Core/RenderBase/Include/VK/VKMemory.h"
#include "Core/Header/Include/Types.h"
#include "Core/DSA/Include/Vector.h"

namespace LD
{

class VKContext;
class VKDevice;
class VKImage;
class VKCommandPool;

struct VKBufferInfo
{
    VkBufferCreateInfo CreateInfo;
    VkMemoryPropertyFlags MemoryProperties;
};

class VKBuffer
{
public:
    VKBuffer();
    VKBuffer(const VKBuffer&) = delete;
    ~VKBuffer();

    VKBuffer& operator=(const VKBuffer&) = delete;

    void Startup(VKDevice& device, const VKBufferInfo& spec);
    void Cleanup();

    inline VkBuffer GetHandle() const
    {
        return mHandle;
    }

    inline u32 GetSize() const
    {
        return mInfo.CreateInfo.size;
    }

    inline VkMemoryRequirements GetMemoryRequirements() const
    {
        return mBufferMemoryRequirements;
    };

    void* Map();
    void Unmap();

    void StageData(u32 dataSize, const void* data, VKCommandPool& transferPool, VkQueue transferQueue);
    void CopyToBuffer(VKBuffer& dstBuffer, VKCommandPool& transferPool, VkQueue transferQueue);
    void CopyToImage(VKImage& dstImage, VkImageSubresourceRange range, const Vector<VkBufferImageCopy>& regions,
                     VKCommandPool& transferPool, VkQueue transferQueue);

private:
    VKDevice* mDevice = nullptr;
    VKBufferInfo mInfo;
    VkBuffer mHandle = VK_NULL_HANDLE;
    VKMemory mBufferMemory;
    VkMemoryRequirements mBufferMemoryRequirements;
};

} // namespace LD