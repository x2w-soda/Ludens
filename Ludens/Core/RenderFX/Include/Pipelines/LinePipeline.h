#pragma once

#include "Core/RenderFX/Include/PrefabPipeline.h"

namespace LD
{

using LineVertex = Vec3;

struct LinePipelineInfo
{
    RDevice Device;
    RPipelineLayout LinePipelineLayout;
    RPass RenderPass;
};

class LinePipeline : public PrefabPipeline
{
public:
    LinePipeline() = default;
    ~LinePipeline();

    void Startup(const LinePipelineInfo& info);
    void Cleanup();

    virtual RPipelineLayoutData GetLayoutData() const override;

private:
    RDevice mDevice;
    RShader mLineVS;
    RShader mLineFS;
};

} // namespace LD