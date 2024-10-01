#pragma once

#include "Core/RenderFX/Include/PrefabPipeline.h"

namespace LD
{

struct SSAOBlurPipelineInfo
{
    RDevice Device;
    RPipelineLayout PipelineLayout;
    RPass RenderPass;
};

class SSAOBlurPipeline : public PrefabPipeline
{
public:
    SSAOBlurPipeline();
    SSAOBlurPipeline(const SSAOBlurPipeline&) = delete;
    ~SSAOBlurPipeline();

    SSAOBlurPipeline& operator=(const SSAOBlurPipeline&) = delete;

    void Startup(const SSAOBlurPipelineInfo& info);
    void Cleanup();

    virtual RPipelineLayoutData GetLayoutData() const override;

private:
    RDevice mDevice;
    RShader mSSAOBlurVS;
    RShader mSSAOBlurFS;
};

} // namespace LD