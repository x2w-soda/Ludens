#include "Core/RenderFX/Include/FrameBuffers/ColorBuffer.h"
#include "Core/RenderFX/Include/Passes/ColorPass.h"

namespace LD
{

void ColorBuffer::Startup(const ColorBufferInfo& info)
{
    LD_DEBUG_ASSERT(info.Device && info.RenderPass);

    mDevice = info.Device;

    RTextureFormat colorFormat = info.RenderPass->GetColorFormat();

    RTextureInfo textureI;
    textureI.Type = RTextureType::Texture2D;
    textureI.Width = info.Width;
    textureI.Height = info.Height;
    textureI.Format = colorFormat;
    textureI.Data = nullptr;
    textureI.Size = info.Width * info.Height * GetTextureFormatPixelSize(colorFormat);
    textureI.TextureUsage = TEXTURE_USAGE_FRAME_BUFFER_ATTACHMENT_BIT;
    textureI.Sampler.AddressMode = RSamplerAddressMode::ClampToEdge;
    textureI.Sampler.MinFilter = RSamplerFilter::Linear;
    textureI.Sampler.MagFilter = RSamplerFilter::Linear;
    mDevice.CreateTexture(mColorAttachment, textureI);

    RFrameBufferInfo frameBufferI;
    frameBufferI.ColorAttachments = { 1, &mColorAttachment };
    frameBufferI.DepthStencilAttachment.Reset();
    frameBufferI.RenderPass = (RPass)*info.RenderPass;
    frameBufferI.Width = info.Width;
    frameBufferI.Height = info.Height;
    mDevice.CreateFrameBuffer(mHandle, frameBufferI);
}

void ColorBuffer::Cleanup()
{
    mDevice.DeleteFrameBuffer(mHandle);
    mDevice.DeleteTexture(mColorAttachment);
    mDevice.ResetHandle();
}

RTexture ColorBuffer::GetColorAttachment() const
{
    LD_DEBUG_ASSERT(mColorAttachment);
    return mColorAttachment;
}

} // namespace LD