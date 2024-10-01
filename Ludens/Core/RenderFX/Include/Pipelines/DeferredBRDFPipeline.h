#pragma once

#include "Core/RenderBase/Include/RDevice.h"
#include "Core/RenderBase/Include/RPipeline.h"
#include "Core/RenderFX/Include/PrefabPipeline.h"

namespace LD
{

struct DeferredBRDFPipelineInfo
{
    RDevice Device;
    RPass RenderPass;
    RPipelineLayout PipelineLayout;
};

class DeferredBRDFPipeline : public PrefabPipeline
{
public:
    void Startup(const DeferredBRDFPipelineInfo& info);
    void Cleanup();

    virtual RPipelineLayoutData GetLayoutData() const override;

private:
    RDevice mDevice;
    RShader mDeferredBRDFVS;
    RShader mDeferredBRDFFS;
};

} // namespace LD