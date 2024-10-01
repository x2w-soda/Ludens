#include "Core/DSA/Include/Array.h"
#include "Core/RenderService/Lib/PipelineResources.h"
#include "Core/RenderService/Lib/RenderPassResources.h"
#include "Core/RenderService/Lib/BindingGroupResources.h"

namespace LD
{

void PipelineResources::Startup(RDevice device, RenderPassResources* passRes, BindingGroupResources* groupRes)
{
    LD_DEBUG_ASSERT(device && passRes && groupRes);
    mDevice = device;
    mPassRes = passRes;
    mGroupRes = groupRes;
}

void PipelineResources::Cleanup()
{
    if (mRect)
        mRect.Cleanup();

    if (mGBuffer)
        mGBuffer.Cleanup();

    if (mCubemap)
        mCubemap.Cleanup();

    if (mDeferredBlinnPhong)
        mDeferredBlinnPhong.Cleanup();

    if (mDeferredBRDF)
        mDeferredBRDF.Cleanup();

    if (mDeferredSSAO)
        mDeferredSSAO.Cleanup();

    if (mSSAOBlur)
        mSSAOBlur.Cleanup();

    if (mSwapChainTransfer)
        mSwapChainTransfer.Cleanup();

    if (mToneMapping)
        mToneMapping.Cleanup();
    
    mGroupRes = nullptr;
    mPassRes = nullptr;
    mDevice.ResetHandle();
}

GBufferPipeline& PipelineResources::GetGBufferPipeline()
{
    if (!mGBuffer)
    {
        Array<RBindingGroupLayout, 2> groupLayout;
        groupLayout[0] = mGroupRes->GetViewportBGL();
        groupLayout[1] = mGroupRes->GetMaterialBGL();

        GBufferPipelineInfo pipelineI;
        pipelineI.Device = mDevice;
        pipelineI.RenderPass = (RPass)mPassRes->GetGBufferPass();
        pipelineI.GBufferPipelineLayout.GroupLayouts = groupLayout.GetView();
        mGBuffer.Startup(pipelineI);
    }

    LD_DEBUG_ASSERT(mGBuffer);
    return mGBuffer;
}

CubemapPipeline& PipelineResources::GetCubemapPipeline()
{
    if (!mCubemap)
    {
        Array<RBindingGroupLayout, 3> groupLayout;
        groupLayout[0] = mGroupRes->GetFrameStaticBGL();
        groupLayout[1] = mGroupRes->GetViewportBGL();
        groupLayout[2] = mGroupRes->GetCubemapBGL();

        CubemapPipelineInfo pipelineI;
        pipelineI.Device = mDevice;
        pipelineI.RenderPass = (RPass)mPassRes->GetGBufferPass();
        pipelineI.CubemapPipelineLayout.GroupLayouts = groupLayout.GetView();
        mCubemap.Startup(pipelineI);
    }

    LD_DEBUG_ASSERT(mCubemap);
    return mCubemap;
}

RectPipeline& PipelineResources::GetRectPipeline()
{
    if (!mRect)
    {
        Array<RBindingGroupLayout, 2> groupLayout;
        groupLayout[0] = mGroupRes->GetViewportBGL();
        groupLayout[1] = mGroupRes->GetRectBGL();

        RectPipelineInfo pipelineI;
        pipelineI.Device = mDevice;
        pipelineI.RenderPass = (RPass)mPassRes->GetColorPassLDR();
        pipelineI.RectPipelineLayout.GroupLayouts = groupLayout.GetView();
        mRect.Startup(pipelineI);
    }

    LD_DEBUG_ASSERT(mRect);
    return mRect;
}

DeferredBlinnPhongPipeline& PipelineResources::GetDeferredBlinnPhongPipeline()
{
    if (!mDeferredBlinnPhong)
    {
        Array<RBindingGroupLayout, 2> groupLayout;
        groupLayout[0] = mGroupRes->GetFrameStaticBGL();
        groupLayout[1] = mGroupRes->GetViewportBGL();

        DeferredBlinnPhongPipelineInfo pipelineI;
        pipelineI.Device = mDevice;
        pipelineI.RenderPass = (RPass)mPassRes->GetColorPassHDR();
        pipelineI.PipelineLayout.GroupLayouts = groupLayout.GetView();
        mDeferredBlinnPhong.Startup(pipelineI);
    }

    LD_DEBUG_ASSERT(mDeferredBlinnPhong);
    return mDeferredBlinnPhong;
}

DeferredBRDFPipeline& PipelineResources::GetDeferredBRDFPipeline()
{
    if (!mDeferredBRDF)
    {
        Array<RBindingGroupLayout, 2> groupLayout;
        groupLayout[0] = mGroupRes->GetFrameStaticBGL();
        groupLayout[1] = mGroupRes->GetViewportBGL();

        DeferredBRDFPipelineInfo pipelineI;
        pipelineI.Device = mDevice;
        pipelineI.RenderPass = (RPass)mPassRes->GetColorPassHDR();
        pipelineI.PipelineLayout.GroupLayouts = groupLayout.GetView();
        mDeferredBRDF.Startup(pipelineI);
    }

    LD_DEBUG_ASSERT(mDeferredBRDF);
    return mDeferredBRDF;
}

DeferredSSAOPipeline& PipelineResources::GetDeferredSSAOPipeline()
{
    if (!mDeferredSSAO)
    {
        Array<RBindingGroupLayout, 2> groupLayout;
        groupLayout[0] = mGroupRes->GetViewportBGL();
        groupLayout[1] = mGroupRes->GetSSAOBGL();

        DeferredSSAOPipelineInfo pipelineI;
        pipelineI.Device = mDevice;
        pipelineI.RenderPass = (RPass)mPassRes->GetSSAOPass();
        pipelineI.PipelineLayout.GroupLayouts = groupLayout.GetView();
        mDeferredSSAO.Startup(pipelineI);
    }

    LD_DEBUG_ASSERT(mDeferredSSAO);
    return mDeferredSSAO;
}

SSAOBlurPipeline& PipelineResources::GetSSAOBlurPipeline()
{
    if (!mSSAOBlur)
    {
        Array<RBindingGroupLayout, 2> groupLayout;
        groupLayout[0] = mGroupRes->GetViewportBGL();
        groupLayout[1] = mGroupRes->GetSSAOBGL();

        SSAOBlurPipelineInfo pipelineI;
        pipelineI.Device = mDevice;
        pipelineI.RenderPass = (RPass)mPassRes->GetSSAOPass();
        pipelineI.PipelineLayout.GroupLayouts = groupLayout.GetView();
        mSSAOBlur.Startup(pipelineI);
    }

    LD_DEBUG_ASSERT(mSSAOBlur);
    return mSSAOBlur;
}

ToneMappingPipeline& PipelineResources::GetToneMappingPipeline()
{
    if (!mToneMapping)
    {
        Array<RBindingGroupLayout, 4> groupLayout;
        groupLayout[0] = mGroupRes->GetFrameStaticBGL();
        groupLayout[1] = mGroupRes->GetViewportBGL();
        groupLayout[2] = mGroupRes->GetViewportBGL();
        groupLayout[3] = mGroupRes->GetToneMappingBGL();

        ToneMappingPipelineInfo pipelineI;
        pipelineI.Device = mDevice;
        pipelineI.RenderPass = (RPass)mPassRes->GetColorPassLDR();
        pipelineI.PipelineLayout.GroupLayouts = groupLayout.GetView();
        mToneMapping.Startup(pipelineI);
    }

    LD_DEBUG_ASSERT(mToneMapping);
    return mToneMapping;
}

SwapChainTransferPipeline& PipelineResources::GetSwapChainTransferPipeline()
{
    if (!mSwapChainTransfer)
    {
        Array<RBindingGroupLayout, 2> groupLayout;
        groupLayout[0] = mGroupRes->GetFrameStaticBGL();
        groupLayout[1] = mGroupRes->GetViewportBGL();

        SwapChainTransferPipelineInfo pipelineI;
        pipelineI.Device = mDevice;
        pipelineI.RenderPass = (RPass)mPassRes->GetSwapChainRenderPass();
        pipelineI.PipelineLayout.GroupLayouts = groupLayout.GetView();
        mSwapChainTransfer.Startup(pipelineI);
    }

    LD_DEBUG_ASSERT(mSwapChainTransfer);
    return mSwapChainTransfer;
}

} // namespace LD