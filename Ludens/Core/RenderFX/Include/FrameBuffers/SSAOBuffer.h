#pragma once

#include "Core/RenderBase/Include/RDevice.h"
#include "Core/RenderBase/Include/RTexture.h"
#include "Core/RenderBase/Include/RFrameBuffer.h"
#include "Core/RenderBase/Include/RPass.h"
#include "Core/RenderFX/Include/PrefabFrameBuffer.h"

namespace LD
{

struct SSAOBufferInfo
{
    RDevice Device;
    RPass RenderPass;
    int Width;
    int Height;
};

class SSAOBuffer : public PrefabFrameBuffer
{
public:
    SSAOBuffer();
    ~SSAOBuffer();

    void Startup(const SSAOBufferInfo& info);
    void Cleanup();

    inline RTexture GetTexture() const
    {
        return mSSAOTexture;
    }

private:
    RDevice mDevice;
    RTexture mSSAOTexture;
};

} // namespace LD