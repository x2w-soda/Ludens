#pragma once

#include "Core/RenderFX/Include/PrefabPipeline.h"

namespace LD
{

struct SwapChainTransferPipelineInfo
{
    RDevice Device;
    RPass RenderPass;
    RPipelineLayout PipelineLayout;
};

class SwapChainTransferPipeline : public PrefabPipeline
{
public:
    void Startup(const SwapChainTransferPipelineInfo& info);
    void Cleanup();

    virtual RPipelineLayoutData GetLayoutData() const override;

private:
    RShader mSwapChainTransferVS;
    RShader mSwapChainTransferFS;
    RDevice mDevice;
};

} // namespace LD