#pragma once

#include "Core/RenderBase/Include/RTexture.h"
#include "Core/RenderFX/Include/PrefabBindingGroup.h"

namespace LD
{

class CubemapGroup : public PrefabBindingGroup
{
public:
    /// @brief startup the cubemap binding group
    /// @param device the owner device
    /// @param cubemapBGL a layout compatible with the cubemap binding group, such as from CreateLayout()
    /// @param cubemap a valid cubemap texture that will be bound to this group at binding 0
    void Startup(RDevice device, RBindingGroupLayout cubemapBGL, RTexture cubemap);

    /// cleanup the cubemap binding group, the input BGL during Startup() is not deleted
    void Cleanup();

    virtual RBindingGroupLayoutData GetLayoutData() const override;

    virtual RBindingGroupLayout CreateLayout(RDevice device) override;

private:
    RDevice mDevice;
};

} // namespace LD