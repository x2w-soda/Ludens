#include <iostream>
#include <algorithm>
#include "Core/RenderBase/Include/VK/VKFrameBuffer.h"
#include "Core/RenderBase/Include/VK/VKContext.h"

namespace LD
{

VKFrameBuffer::~VKFrameBuffer()
{
    LD_DEBUG_ASSERT(mHandle == VK_NULL_HANDLE);
    LD_DEBUG_ASSERT(mDevice == VK_NULL_HANDLE);
}

void VKFrameBuffer::Startup(const VKDevice& device, const VKFrameBufferInfo& info)
{
    mDevice = device.GetHandle();

    // unwrap vulkan image view handles
    Vector<VkImageView> handles(info.Attachments.Size());
    std::transform(info.Attachments.Begin(), info.Attachments.End(), handles.Begin(),
                   [](const Ref<VKImageView>& wrapper) { return wrapper->GetHandle(); });

    VkFramebufferCreateInfo frameBufferCI{};
    frameBufferCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    frameBufferCI.renderPass = info.RenderPass;
    frameBufferCI.attachmentCount = handles.Size();
    frameBufferCI.pAttachments = handles.Data();
    frameBufferCI.width = info.Extent.width;
    frameBufferCI.height = info.Extent.height;
    frameBufferCI.layers = 1;

    VK_ASSERT(vkCreateFramebuffer(mDevice, &frameBufferCI, nullptr, &mHandle));
}

void VKFrameBuffer::Cleanup()
{
    vkDestroyFramebuffer(mDevice, mHandle, nullptr);
    mDevice = VK_NULL_HANDLE;
    mHandle = VK_NULL_HANDLE;
}

} // namespace LD