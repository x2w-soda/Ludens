#pragma once

#include "Core/Header/Include/Types.h"
#include "Core/RenderBase/Include/RBuffer.h"
#include "Core/RenderFX/Include/PrefabBindingGroup.h"

namespace LD
{

struct ToneMappingUBO
{
    i32 LDRResult;
};

class ToneMappingGroup : public PrefabBindingGroup
{
public:
    ToneMappingGroup() = default;
    ~ToneMappingGroup();

    void Startup(RDevice device, RBindingGroupLayout toneMappingBGL);
    void Cleanup();

    RBuffer GetUBO()
    {
        return mUBO;
    }

    virtual RBindingGroupLayoutData GetLayoutData() const override;

    virtual RBindingGroupLayout CreateLayout(RDevice device) override;

private:
    RDevice mDevice;
    RBuffer mUBO;
};

} // namespace LD