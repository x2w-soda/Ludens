#pragma once

#include "Core/RenderFX/Include/Pipelines/GBufferPipeline.h"
#include "Core/RenderFX/Include/Pipelines/CubemapPipeline.h"
#include "Core/RenderFX/Include/Pipelines/RectPipeline.h"
#include "Core/RenderFX/Include/Pipelines/DeferredBlinnPhongPipeline.h"
#include "Core/RenderFX/Include/Pipelines/DeferredBRDFPipeline.h"
#include "Core/RenderFX/Include/Pipelines/DeferredSSAOPipeline.h"
#include "Core/RenderFX/Include/Pipelines/SSAOBlurPipeline.h"
#include "Core/RenderFX/Include/Pipelines/SwapChainTransferPipeline.h"
#include "Core/RenderFX/Include/Pipelines/PostProcess/ToneMappingPipeline.h"
#include "Core/RenderService/Lib/RenderResources.h"

namespace LD
{

class RenderPassResources;
class BindingGroupResources;

class PipelineResources : public RenderResources
{
public:
    void Startup(RDevice device, RenderPassResources* passRes, BindingGroupResources* groupRes);
    void Cleanup();

    GBufferPipeline& GetGBufferPipeline();

    CubemapPipeline& GetCubemapPipeline();

    RectPipeline& GetRectPipeline();

    DeferredBlinnPhongPipeline& GetDeferredBlinnPhongPipeline();

    DeferredBRDFPipeline& GetDeferredBRDFPipeline();

    DeferredSSAOPipeline& GetDeferredSSAOPipeline();

    SSAOBlurPipeline& GetSSAOBlurPipeline();

    ToneMappingPipeline& GetToneMappingPipeline();

    SwapChainTransferPipeline& GetSwapChainTransferPipeline();

private:
    RenderPassResources* mPassRes;
    BindingGroupResources* mGroupRes;
    GBufferPipeline mGBuffer;
    CubemapPipeline mCubemap;
    RectPipeline mRect;
    DeferredBlinnPhongPipeline mDeferredBlinnPhong;
    DeferredBRDFPipeline mDeferredBRDF;
    DeferredSSAOPipeline mDeferredSSAO;
    SSAOBlurPipeline mSSAOBlur;
    SwapChainTransferPipeline mSwapChainTransfer;
    ToneMappingPipeline mToneMapping;
};

} // namespace LD