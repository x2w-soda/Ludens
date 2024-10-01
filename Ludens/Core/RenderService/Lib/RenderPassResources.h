#pragma once

#include <unordered_map>
#include "Core/RenderFX/Include/Passes/GBufferPass.h"
#include "Core/RenderFX/Include/Passes/SSAOPass.h"
#include "Core/RenderFX/Include/Passes/ColorPass.h"
#include "Core/RenderService/Lib/RenderResources.h"
#include "Core/RenderService/Lib/RenderPassResources.h"

namespace LD
{

class RenderPassResources : public RenderResources
{
public:
    void Startup(RDevice device);
    void Cleanup();

    RPass GetSwapChainRenderPass();
    GBufferPass& GetGBufferPass();
    SSAOPass& GetSSAOPass();
    ColorPass& GetColorPassHDR();
    ColorPass& GetColorPassLDR();

private:
    RPass mSwapChainRenderPass;
    GBufferPass mGBufferPass;
    SSAOPass mSSAOPass;
    ColorPass mColorPassHDR;
    ColorPass mColorPassLDR;
};

} // namespace LD