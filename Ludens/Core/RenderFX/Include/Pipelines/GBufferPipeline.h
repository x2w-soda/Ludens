#pragma once

#include "Core/RenderBase/Include/RDevice.h"
#include "Core/RenderBase/Include/RPipeline.h"
#include "Core/RenderFX/Include/PrefabPipeline.h"

namespace LD
{

struct GBufferPipelineInfo
{
    RDevice Device;
    RPipelineLayout GBufferPipelineLayout;
    RPass RenderPass;
};

class GBufferPipeline : public PrefabPipeline
{
public:
    GBufferPipeline();
    GBufferPipeline(const GBufferPipeline&) = delete;
    ~GBufferPipeline();

    GBufferPipeline& operator=(const GBufferPipeline&) = delete;

    void Startup(const GBufferPipelineInfo& info);
    void Cleanup();

    virtual RPipelineLayoutData GetLayoutData() const override;

private:
    RDevice mDevice;
    RShader mGBufferVS;
    RShader mGBufferFS;
};

} // namespace LD