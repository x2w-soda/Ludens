#pragma once

#include "Core/RenderFX/Include/PrefabPipeline.h"

namespace LD
{

struct FXAAPipelineInfo
{
    RDevice Device;
    RPipelineLayout FXAAPipelineLayout;
    RPass RenderPass;
};

class FXAAPipeline : public PrefabPipeline
{
public:
    void Startup(const FXAAPipelineInfo& info);
    void Cleanup();

    virtual RPipelineLayoutData GetLayoutData() const override;

private:
    RDevice mDevice;
    RShader mFXAAVS;
    RShader mFXAAFS;
};

} // namespace LD