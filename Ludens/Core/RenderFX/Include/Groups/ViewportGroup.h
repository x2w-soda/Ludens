#pragma once

#include "Core/RenderBase/Include/RBinding.h"
#include "Core/RenderBase/Include/RBuffer.h"
#include "Core/RenderFX/Include/PrefabBindingGroup.h"
#include "Core/Math/Include/Mat4.h"

namespace LD
{

class GBuffer;
class ColorBuffer;

/// binding 0, viewport information
struct ViewportUBO
{
    alignas(16) Mat4 ViewMat;         // camera view matrix
    alignas(16) Mat4 ProjMat;         // camera projection matrix
    alignas(16) Mat4 ViewProjMat;     // pre-computed at CPU side
    alignas(16) Vec3 ViewPos;         // world space eye position
    alignas(8) Vec2 Size;             // width and height (and aspect ratio) of the viewport
    alignas(4) int PointLightStart;   // starting index in FrameStaticGroup point light array
    alignas(4) int PointLightCount;   // number of point lights participating in lighting
};

LD_STATIC_ASSERT(sizeof(ViewportUBO) == 64 * 3 + 16 + 8 + 4 + 4);

/// Standard Viewport Binding Group, this group provides information of how the scene is viewed from.
/// A local split-screen game with 2 players might require multiple viewports for each camera.
/// A shadow mapping pass might require multiple viewports for each light source.
class ViewportGroup : public PrefabBindingGroup
{
public:
    ViewportGroup() = default;
    ViewportGroup(const ViewportGroup&) = delete;
    ~ViewportGroup();

    ViewportGroup& operator=(const ViewportGroup&) = delete;

    /// @brief startup the viewport binding group
    /// @param device the owner device
    /// @param viewportBGL a layout compatible with the viewport binding group, such as from CreateLayout()
    void Startup(RDevice device, RBindingGroupLayout viewportBGL);

    /// cleanup the viewport binding group, the input BGL during Startup() is not deleted
    void Cleanup();

    /// bind the gbuffer textures to this viewport,
    /// the textures are bound at bindings 1 to 3 of this group.
    void BindGBuffer(const GBuffer& gbuffer);

    /// bind the ssao texture to this viewport,
    /// the texture is bound at binding 4 of this group.
    void BindSSAOTexture(RTexture ssao);

    /// bind the HDR and LDR color buffers to this viewport
    /// the textures are bound at binding 1 and 2 of this group.
    void BindColorTextures(const ColorBuffer& hdr, const ColorBuffer& ldr);

    virtual RBindingGroupLayoutData GetLayoutData() const override;

    /// create a corresponding layout for the viewport binding group
    virtual RBindingGroupLayout CreateLayout(RDevice device) override;

    inline RBuffer GetUBO() const
    {
        LD_DEBUG_ASSERT(mUBO);
        return mUBO;
    }

private:
    RDevice mDevice;
    RBuffer mUBO;
};

} // namespace LD