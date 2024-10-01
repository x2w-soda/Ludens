#pragma once

#include "Core/RenderBase/Include/RBuffer.h"
#include "Core/RenderBase/Include/RTexture.h"
#include "Core/RenderFX/Include/PrefabBindingGroup.h"

namespace LD
{

/// group of resources used during ssao texture generation
class SSAOGroup : public PrefabBindingGroup
{
public:
    SSAOGroup() = default;
    SSAOGroup(const SSAOGroup&) = delete;
    ~SSAOGroup();

    SSAOGroup& operator=(const SSAOGroup&) = delete;

    /// @brief startup the ssao binding group
    /// @param device the owning device
    /// @param ssaoBGL a layout compatible with the ssao binding group, such as from CreateLayout()
    void Startup(RDevice device, RBindingGroupLayout ssaoBGL);

    /// cleanup the ssao binding group, the input BGL in Startup() is not deleted
    void Cleanup();

    /// bind the raw output of ssao pipeline to this binding group
    void BindSSAOTexture(RTexture ssao);

    virtual RBindingGroupLayoutData GetLayoutData() const override;

    virtual RBindingGroupLayout CreateLayout(RDevice device) override;

private:
    RDevice mDevice;
    RBuffer mKernelUBO;
    RTexture mNoise;
};

} // namespace LD