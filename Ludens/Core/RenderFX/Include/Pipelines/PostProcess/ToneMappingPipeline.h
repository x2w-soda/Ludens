#pragma once

#include "Core/RenderFX/Include/PrefabPipeline.h"

namespace LD
{

struct ToneMappingPipelineInfo
{
    RDevice Device;
    RPass RenderPass;
    RPipelineLayout PipelineLayout;
};

class ToneMappingPipeline : public PrefabPipeline
{
public:
    void Startup(const ToneMappingPipelineInfo& info);
    void Cleanup();

    virtual RPipelineLayoutData GetLayoutData() const override;

private:
    RShader mToneMappingVS;
    RShader mToneMappingFS;
    RDevice mDevice;
};

} // namespace LD