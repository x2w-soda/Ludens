#pragma once

#include "Core/RenderBase/Include/RDevice.h"
#include "Core/RenderBase/Include/RPipeline.h"
#include "Core/RenderFX/Include/PrefabPipeline.h"

namespace LD {

struct DeferredBlinnPhongPipelineInfo
{
    RDevice Device;
    RPipelineLayout PipelineLayout;
    RPass RenderPass;
};

class DeferredBlinnPhongPipeline : public PrefabPipeline
{
public:
    DeferredBlinnPhongPipeline();
    DeferredBlinnPhongPipeline(const DeferredBlinnPhongPipeline&) = delete;
    ~DeferredBlinnPhongPipeline();

    DeferredBlinnPhongPipeline& operator=(const DeferredBlinnPhongPipeline&) = delete;

    void Startup(const DeferredBlinnPhongPipelineInfo& info);
    void Cleanup();

    virtual RPipelineLayoutData GetLayoutData() const override;

private:
    RDevice mDevice;
    RShader mDeferredBlinnPhongVS;
    RShader mDeferredBlinnPhongFS;
};

} // namespace LD