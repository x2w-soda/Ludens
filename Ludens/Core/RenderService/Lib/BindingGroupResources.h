#pragma once

#include "Core/RenderBase/Include/RBinding.h"
#include "Core/RenderFX/Include/Groups/FrameStaticGroup.h"
#include "Core/RenderFX/Include/Groups/SSAOGroup.h"
#include "Core/RenderFX/Include/Groups/ToneMappingGroup.h"
#include "Core/RenderService/Lib/RenderResources.h"

namespace LD
{

class BindingGroupResources : public RenderResources
{
public:
    void Startup(RDevice device);
    void Cleanup();

    FrameStaticGroup& GetFrameStaticGroup();

    ToneMappingGroup& GetToneMappingGroup();

    SSAOGroup& GetSSAOGroup();

    inline RBindingGroupLayout GetFrameStaticBGL()
    {
        return mFrameStaticBGL;
    }

    inline RBindingGroupLayout GetToneMappingBGL()
    {
        return mToneMappingBGL;
    }

    inline RBindingGroupLayout GetViewportBGL()
    {
        return mViewportBGL;
    }

    inline RBindingGroupLayout GetMaterialBGL()
    {
        return mMaterialBGL;
    }

    inline RBindingGroupLayout GetCubemapBGL()
    {
        return mCubemapBGL;
    }

    inline RBindingGroupLayout GetRectBGL()
    {
        return mRectBGL;
    }

    inline RBindingGroupLayout GetSSAOBGL()
    {
        return mSSAOBGL;
    }

private:
    FrameStaticGroup mFrameStaticGroup;
    ToneMappingGroup mToneMappingGroup;
    SSAOGroup mSSAOGroup;
    RBindingGroupLayout mFrameStaticBGL;
    RBindingGroupLayout mToneMappingBGL;
    RBindingGroupLayout mViewportBGL;
    RBindingGroupLayout mMaterialBGL;
    RBindingGroupLayout mCubemapBGL;
    RBindingGroupLayout mRectBGL;
    RBindingGroupLayout mSSAOBGL;
};

} // namespace LD