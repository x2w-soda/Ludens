#include "Core/RenderFX/Include/FrameBuffers/GBuffer.h"
#include "Core/RenderFX/Include/FrameBuffers/SSAOBuffer.h"
#include "Core/RenderFX/Include/FrameBuffers/ColorBuffer.h"
#include "Core/RenderFX/Include/Passes/SSAOPass.h"
#include "Core/RenderFX/Include/Passes/ColorPass.h"
#include "Core/RenderService/Lib/FrameBufferResources.h"
#include "Core/RenderService/Lib/RenderPassResources.h"

namespace LD
{

void FrameBufferResources::Startup(RDevice device, RenderPassResources* passRes)
{
    LD_DEBUG_ASSERT(device && passRes);
    mDevice = device;
    mPassRes = passRes;
}

void FrameBufferResources::Cleanup()
{
    mDevice.ResetHandle();
    mPassRes = nullptr;
}

void FrameBufferResources::CreateGBuffer(GBuffer& gbuffer, int width, int height)
{
    LD_DEBUG_ASSERT(!gbuffer && width > 0 && height > 0);

    GBufferPass& pass = mPassRes->GetGBufferPass();

    GBufferInfo gbufferI;
    gbufferI.Device = mDevice;
    gbufferI.RenderPass = (RPass)pass;
    gbufferI.PositionFormat = pass.GetPositionFormat();
    gbufferI.NormalsFormat = pass.GetNormalFormat();
    gbufferI.AlbedoFormat = pass.GetAlbedoFormat();
    gbufferI.DepthStencilFormat = pass.GetDepthStencilFormat();
    gbufferI.Width = (u32)width;
    gbufferI.Height = (u32)height;

    gbuffer.Startup(gbufferI);
    LD_DEBUG_ASSERT(gbuffer);
}

void FrameBufferResources::CreateSSAOBuffer(SSAOBuffer& buffer, int width, int height, SSAOPass* pass)
{
    LD_DEBUG_ASSERT(!buffer && width > 0 && height > 0);

    SSAOBufferInfo bufferI;
    bufferI.Device = mDevice;
    bufferI.RenderPass = (RPass)*pass;
    bufferI.Width = width;
    bufferI.Height = height;

    buffer.Startup(bufferI);
    LD_DEBUG_ASSERT(buffer);
}

void FrameBufferResources::CreateColorBuffer(ColorBuffer& buffer, int width, int height, ColorPass* pass)
{
    LD_DEBUG_ASSERT(!buffer && width > 0 && height > 0);

    ColorBufferInfo bufferI;
    bufferI.Device = mDevice;
    bufferI.RenderPass = pass;
    bufferI.Width = width;
    bufferI.Height = height;

    buffer.Startup(bufferI);
    LD_DEBUG_ASSERT(buffer);
}

} // namespace LD