#include "Core/RenderFX/Include/FrameBuffers/SSAOBuffer.h"

namespace LD
{

SSAOBuffer::SSAOBuffer()
{
}

SSAOBuffer::~SSAOBuffer()
{
    LD_DEBUG_ASSERT(!mDevice);
}

void SSAOBuffer::Startup(const SSAOBufferInfo& info)
{
    mDevice = info.Device;

    // the SSAO texture is a grayscale texture that describes the occlusion of
    // the first visible fragment from camera
    RTextureInfo textureI{};
    textureI.Type = RTextureType::Texture2D;
    textureI.TextureUsage = TEXTURE_USAGE_FRAME_BUFFER_ATTACHMENT_BIT;
    textureI.Width = info.Width;
    textureI.Height = info.Height;
    textureI.Format = RTextureFormat::R8;
    textureI.Size = info.Width * info.Height;
    textureI.Data = nullptr;
    textureI.Sampler.AddressMode = RSamplerAddressMode::ClampToEdge;
    mDevice.CreateTexture(mSSAOTexture, textureI);

    RFrameBufferInfo ssaoI{};
    ssaoI.Width = info.Width;
    ssaoI.Height = info.Height;
    ssaoI.RenderPass = info.RenderPass;
    ssaoI.ColorAttachments = { 1, &mSSAOTexture };
    mDevice.CreateFrameBuffer(mHandle, ssaoI);
}

void SSAOBuffer::Cleanup()
{
    mDevice.DeleteFrameBuffer(mHandle);
    mDevice.DeleteTexture(mSSAOTexture);
    mDevice.ResetHandle();
}



} // namespace LD