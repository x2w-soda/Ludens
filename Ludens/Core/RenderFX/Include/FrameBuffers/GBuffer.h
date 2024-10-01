#pragma once

#include "Core/RenderBase/Include/RDevice.h"
#include "Core/RenderBase/Include/RTexture.h"
#include "Core/RenderBase/Include/RFrameBuffer.h"
#include "Core/RenderBase/Include/RPass.h"
#include "Core/RenderFX/Include/PrefabFrameBuffer.h"

namespace LD
{

struct GBufferInfo
{
    RDevice Device;
    u32 Width;
    u32 Height;
    RPass RenderPass;
    RTextureFormat PositionFormat = RTextureFormat::RGBA16F;
    RTextureFormat NormalsFormat = RTextureFormat::RGBA16F;
    RTextureFormat AlbedoFormat = RTextureFormat::RGBA8;
    RTextureFormat DepthStencilFormat = RTextureFormat::Undefined;
};

// template GBuffer with common geometry pass attachments
// - default RGBA16F Position
// - default RGBA16F Normals
// - default RGBA8 Albedo Color
class GBuffer : public PrefabFrameBuffer
{
public:
    GBuffer();
    ~GBuffer();

    void Startup(const GBufferInfo& info);
    void Cleanup();

    inline RTexture GetPosition() const
    {
        LD_DEBUG_ASSERT(mPosition);
        return mPosition;
    }

    inline RTexture GetNormals() const
    {
        LD_DEBUG_ASSERT(mNormals);
        return mNormals;
    }

    inline RTexture GetAlbedo() const
    {
        LD_DEBUG_ASSERT(mAlbedo);
        return mAlbedo;
    }

    inline RTexture GetDepthStencil() const
    {
        LD_DEBUG_ASSERT(mDepthStencil);
        return mDepthStencil;
    }

private:
    RDevice mDevice;
    RTexture mPosition;
    RTexture mNormals;
    RTexture mAlbedo;
    RTexture mDepthStencil;
};

} // namespace LD