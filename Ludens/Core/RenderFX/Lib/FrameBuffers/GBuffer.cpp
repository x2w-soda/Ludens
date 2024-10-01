#include "Core/RenderFX/Include/FrameBuffers/GBuffer.h"
#include "Core/RenderBase/Include/VK/VKFormat.h"

namespace LD {

	GBuffer::GBuffer()
	{
	}

	GBuffer::~GBuffer()
	{
		LD_DEBUG_ASSERT(!mDevice);
	}

	void GBuffer::Startup(const GBufferInfo& info)
	{
		mDevice = info.Device;

		RTextureInfo textureI{};
		textureI.Type = RTextureType::Texture2D;
		textureI.TextureUsage = TEXTURE_USAGE_FRAME_BUFFER_ATTACHMENT_BIT;
		textureI.Width = info.Width;
		textureI.Height = info.Height;
		textureI.Format = info.PositionFormat;
		textureI.Size = info.Width * info.Height * GetTextureFormatPixelSize(info.PositionFormat);
		textureI.Data = nullptr;
		textureI.Sampler.AddressMode = RSamplerAddressMode::ClampToEdge;
		mDevice.CreateTexture(mPosition, textureI);

		textureI.Format = info.NormalsFormat;
		textureI.Size = info.Width * info.Height * GetTextureFormatPixelSize(info.NormalsFormat);
		mDevice.CreateTexture(mNormals, textureI);

		textureI.Format = info.AlbedoFormat;
		textureI.Size = info.Width * info.Height * GetTextureFormatPixelSize(info.AlbedoFormat);
		mDevice.CreateTexture(mAlbedo, textureI);

		if (info.DepthStencilFormat != RTextureFormat::Undefined)
		{
			textureI.Format = info.DepthStencilFormat;
			textureI.Size = info.Width * info.Height * GetTextureFormatPixelSize(info.AlbedoFormat);
			mDevice.CreateTexture(mDepthStencil, textureI);
		}

		RTexture colorAttachments[3] = {
			mPosition, mNormals, mAlbedo
		};

		RFrameBufferInfo gbufferI{};
		gbufferI.Width = info.Width;
		gbufferI.Height = info.Height;
		gbufferI.RenderPass = info.RenderPass;
		gbufferI.ColorAttachments = { 3, colorAttachments };
		if (mDepthStencil)
			gbufferI.DepthStencilAttachment = mDepthStencil;

		mDevice.CreateFrameBuffer(mHandle, gbufferI);
	}

	void GBuffer::Cleanup()
	{
		mDevice.DeleteFrameBuffer(mHandle);

		if (mDepthStencil)
			mDevice.DeleteTexture(mDepthStencil);

		mDevice.DeleteTexture(mAlbedo);
		mDevice.DeleteTexture(mNormals);
		mDevice.DeleteTexture(mPosition);

		mDevice.ResetHandle();
	}

} // namespace LD