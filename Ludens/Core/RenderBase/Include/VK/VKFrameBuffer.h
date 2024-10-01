#pragma once

#include <vulkan/vulkan_core.h>
#include "Core/DSA/Include/Vector.h"
#include "Core/OS/Include/Memory.h"
#include "Core/RenderBase/Include/VK/VKImage.h"

namespace LD
{

class VKContext;

struct VKFrameBufferInfo
{
    VkRenderPass RenderPass;
    VkExtent2D Extent;
    Vector<Ref<VKImageView>> Attachments;
};

class VKFrameBuffer
{
public:
    VKFrameBuffer() = default;
    VKFrameBuffer(const VKFrameBuffer&) = delete;
    ~VKFrameBuffer();

    VKFrameBuffer& operator=(const VKFrameBuffer&) = delete;

    inline operator bool() const
    {
        return mHandle != VK_NULL_HANDLE && mDevice != VK_NULL_HANDLE;
    }

    void Startup(const VKDevice& device, const VKFrameBufferInfo& info);
    void Cleanup();

    inline VkFramebuffer GetHandle() const
    {
        return mHandle;
    }

private:
    VkDevice mDevice = VK_NULL_HANDLE;
    VkFramebuffer mHandle = VK_NULL_HANDLE;
};

} // namespace LD