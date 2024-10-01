#pragma once

#include "Core/RenderBase/Include/RDevice.h"
#include "Core/RenderBase/Include/RBinding.h"
#include "Core/RenderBase/Include/RTexture.h"
#include "Core/RenderFX/Include/PrefabBindingGroup.h"

namespace LD
{

/// rect pipeline binding group
class RectGroup : public PrefabBindingGroup
{
public:
    RectGroup() = default;
    RectGroup(const RectGroup&) = delete;
    ~RectGroup();

    RectGroup& operator=(const RectGroup&) = delete;

    /// @brief startup the rect binding group
    /// @param device the owning device
    /// @param rectBGL a layout compatible with the rect binding group, such as from CreateLayout()
    void Startup(RDevice device, RBindingGroupLayout rectBGL);

    /// cleanup the rect binding group, the input BGL in Startup() is not deleted
    void Cleanup();

    /// @brief bind a texture in the rect binding group
    /// @param textureH the texture to bind
    /// @param arrayIndex in the range [0, 15], only 16 texture slots are currently available
    RResult BindTexture(RTexture& textureH, int arrayIndex);

    virtual RBindingGroupLayoutData GetLayoutData() const override;

    virtual RBindingGroupLayout CreateLayout(RDevice device) override;

private:
    RDevice mDevice;
};

} // namespace LD