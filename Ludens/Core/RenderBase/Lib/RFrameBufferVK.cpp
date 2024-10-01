#include "Core/RenderBase/Lib/RFrameBufferVK.h"
#include "Core/RenderBase/Lib/RPassVK.h"
#include "Core/RenderBase/Lib/RDeviceVK.h"

namespace LD
{

RFrameBufferVK::RFrameBufferVK()
{
}

RFrameBufferVK::~RFrameBufferVK()
{
    LD_DEBUG_ASSERT(ID == 0);
}

void RFrameBufferVK::Startup(RFrameBuffer& frameBufferH, const RFrameBufferInfo& info, RDeviceVK& device)
{
    RFrameBufferBase::Startup(frameBufferH, info, (RDeviceBase*)&device);
    VKRenderPass& vkRenderPass = Derive<RPassVK>(info.RenderPass).RenderPass;
    VKContext& vkContext = device.Context;

    VKFrameBufferInfo fbInfo{};
    fbInfo.Extent.width = info.Width;
    fbInfo.Extent.height = info.Height;
    fbInfo.RenderPass = vkRenderPass.GetHandle();
    fbInfo.Attachments.Clear();

    for (const RTexture& textureH : info.ColorAttachments)
    {
        RTextureVK& vkTexture = Derive<RTextureVK>(textureH);
        LD_DEBUG_ASSERT(vkTexture.ImageView != nullptr);

        fbInfo.Attachments.PushBack(vkTexture.ImageView);
    }

    if (info.DepthStencilAttachment.HasValue())
    {
        RTextureVK& vkTexture = Derive<RTextureVK>(info.DepthStencilAttachment.Value());
        LD_DEBUG_ASSERT(vkTexture.ImageView != nullptr);

        fbInfo.Attachments.PushBack(vkTexture.ImageView);
    }

    FrameBuffer.Startup(vkContext.GetDevice(), fbInfo);
}

void RFrameBufferVK::Cleanup(RFrameBuffer& frameBufferH)
{
    RFrameBufferBase::Cleanup(frameBufferH);

    FrameBuffer.Cleanup();
}

RResult RFrameBufferVK::Invalidate(const RFrameBufferInfo& info)
{
    // TODO:
    return RResult();
}

} // namespace LD