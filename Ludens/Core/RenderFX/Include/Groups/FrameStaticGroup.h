#pragma once

#include "Core/RenderBase/Include/RBinding.h"
#include "Core/RenderBase/Include/RBuffer.h"
#include "Core/RenderFX/Include/PrefabBindingGroup.h"

#define LD_MAX_POINT_LIGHTS 128

namespace LD
{

struct DirectionalLightData
{
    Vec4 Dir;
    Vec4 Color;
};

LD_STATIC_ASSERT(sizeof(DirectionalLightData) == 32);

struct PointLightData
{
    Vec4 PosRadius;
    Vec4 Color;
};

LD_STATIC_ASSERT(sizeof(PointLightData) == 32);

/// binding 0, lighting conditions of the current frame
struct FrameStaticLightingUBO
{
    alignas(32) DirectionalLightData DirectionalLight;
    alignas(32) PointLightData PointLights[LD_MAX_POINT_LIGHTS];
};

LD_STATIC_ASSERT(sizeof(FrameStaticLightingUBO) ==
                 sizeof(DirectionalLightData) + sizeof(PointLightData) * LD_MAX_POINT_LIGHTS);

/// resources whose values do not change within a frame.
/// the expected usage is to create one instance of this binding group
/// and bind it at group index 0, only updating its values between frames.
class FrameStaticGroup : public PrefabBindingGroup
{
public:
    FrameStaticGroup() = default;
    FrameStaticGroup(const FrameStaticGroup&) = delete;
    ~FrameStaticGroup();

    FrameStaticGroup& operator=(const FrameStaticGroup&) = delete;

    /// @brief startup the frame static binding group
    /// @param device the owner device
    /// @param frameStaticBGL a layout compatible with the frame static binding group,
    ///        such as one created from CreateLayout()
    void Startup(RDevice device, RBindingGroupLayout frameStaticBGL);

    /// cleanup the frame static binding group, the input BGL during Startup() is not deleted
    void Cleanup();

    virtual RBindingGroupLayoutData GetLayoutData() const override;

    virtual RBindingGroupLayout CreateLayout(RDevice device) override;

    inline RBuffer GetLightingUBO() const
    {
        LD_DEBUG_ASSERT(mLightingUBO);
        return mLightingUBO;
    }

private:
    RDevice mDevice;
    RBuffer mLightingUBO;
};

} // namespace LD