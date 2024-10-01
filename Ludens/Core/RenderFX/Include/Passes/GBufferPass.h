#pragma once

#include "Core/RenderBase/Include/RTexture.h"
#include "Core/RenderFX/Include/PrefabRenderPass.h"

namespace LD
{

struct GBufferPassInfo
{
    RDevice Device;
    RTextureFormat PositionFormat = RTextureFormat::RGBA16F;
    RTextureFormat NormalFormat = RTextureFormat::RGBA16F;
    RTextureFormat AlbedoFormat = RTextureFormat::RGBA8;
    RTextureFormat DepthStencilFormat = RTextureFormat::D32F;
};

class GBufferPass : public PrefabRenderPass
{
public:
    void Startup(const GBufferPassInfo& info);
    void Cleanup();

    inline RTextureFormat GetPositionFormat() const
    {
        return mPositionFormat;
    }

    inline RTextureFormat GetNormalFormat() const
    {
        return mNormalFormat;
    }

    inline RTextureFormat GetAlbedoFormat() const
    {
        return mAlbedoFormat;
    }

    inline RTextureFormat GetDepthStencilFormat() const
    {
        return mDepthStencilFormat;
    }

private:
    RDevice mDevice;
    RTextureFormat mPositionFormat;
    RTextureFormat mNormalFormat;
    RTextureFormat mAlbedoFormat;
    RTextureFormat mDepthStencilFormat;
};

} // namespace LD