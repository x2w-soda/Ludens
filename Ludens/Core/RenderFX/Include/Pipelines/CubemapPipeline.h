#pragma once

#include "Core/RenderFX/Include/PrefabPipeline.h"

namespace LD
{

struct CubemapPipelineInfo
{
    RDevice Device;
    RPipelineLayout CubemapPipelineLayout;
    RPass RenderPass;
};

class CubemapPipeline : public PrefabPipeline
{
public:
    void Startup(const CubemapPipelineInfo& info);
    void Cleanup();

	virtual RPipelineLayoutData GetLayoutData() const override;

private:
    RDevice mDevice;
    RShader mCubemapVS;
    RShader mCubemapFS;
};

} // namespace LD