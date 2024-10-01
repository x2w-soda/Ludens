#pragma once

#include "Core/RenderBase/Include/RDevice.h"
#include "Core/RenderBase/Include/RPipeline.h"
#include "Core/RenderFX/Include/PrefabPipeline.h"

namespace LD
{

struct DeferredSSAOPipelineInfo
{
    RDevice Device;
    RPipelineLayout PipelineLayout;
    RPass RenderPass;
};

class DeferredSSAOPipeline : public PrefabPipeline
{
public:
    DeferredSSAOPipeline();
    DeferredSSAOPipeline(const DeferredSSAOPipeline&) = delete;
    ~DeferredSSAOPipeline();

    DeferredSSAOPipeline& operator=(const DeferredSSAOPipeline&) = delete;

    void Startup(const DeferredSSAOPipelineInfo& info);
    void Cleanup();

    virtual RPipelineLayoutData GetLayoutData() const override;

private:
    RDevice mDevice;
    RShader mDeferredSSAOVS;
    RShader mDeferredSSAOFS;
};

} // namespace LD