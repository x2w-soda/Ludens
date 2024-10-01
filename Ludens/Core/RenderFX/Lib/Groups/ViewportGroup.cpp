#include "Core/DSA/Include/Array.h"
#include "Core/RenderFX/Include/Groups/ViewportGroup.h"
#include "Core/RenderFX/Include/FrameBuffers/GBuffer.h"
#include "Core/RenderFX/Include/FrameBuffers/ColorBuffer.h"

namespace LD {

ViewportGroup::~ViewportGroup()
{
    LD_DEBUG_ASSERT(!mDevice);
}

void ViewportGroup::Startup(RDevice device, RBindingGroupLayout viewportBGL)
{
    LD_DEBUG_ASSERT(device);
    mDevice = device;

    RBufferInfo bufferI;
    bufferI.Type = RBufferType::UniformBuffer;
    bufferI.MemoryUsage = RMemoryUsage::FrameDynamic;
    bufferI.Data = nullptr;
    bufferI.Size = sizeof(ViewportUBO);
    mDevice.CreateBuffer(mUBO, bufferI);

    RBindingGroupInfo bgI;
    bgI.Layout = viewportBGL;
    mDevice.CreateBindingGroup(mHandle, bgI);

    mHandle.BindUniformBuffer(0, mUBO);
}

void ViewportGroup::Cleanup()
{
    mDevice.DeleteBindingGroup(mHandle);
    mDevice.DeleteBuffer(mUBO);
    mDevice.ResetHandle();
}

void ViewportGroup::BindGBuffer(const GBuffer& gbuffer)
{
    mHandle.BindTexture(1, gbuffer.GetPosition());
    mHandle.BindTexture(2, gbuffer.GetNormals());
    mHandle.BindTexture(3, gbuffer.GetAlbedo());
}

void ViewportGroup::BindSSAOTexture(RTexture ssao)
{
    mHandle.BindTexture(4, ssao);
}

void ViewportGroup::BindColorTextures(const ColorBuffer& hdr, const ColorBuffer& ldr)
{
    mHandle.BindTexture(1, hdr.GetColorAttachment());
    mHandle.BindTexture(2, ldr.GetColorAttachment());
}

RBindingGroupLayoutData ViewportGroup::GetLayoutData() const
{
    RBindingInfo binding0;
    binding0.Count = 1;
    binding0.Type = RBindingType::UniformBuffer;

    RBindingInfo texture;
    texture.Count = 1;
    texture.Type = RBindingType::Texture;

    // viewport UBO
    // gbuffer position
    // gbuffer normals
    // gbuffer albedo
    // ssao texture
    return { binding0, texture, texture, texture, texture };
}

RBindingGroupLayout ViewportGroup::CreateLayout(RDevice device)
{
    LD_DEBUG_ASSERT((bool)device);

    RBindingGroupLayout viewportBGL;

    Array<RBindingInfo, 5> bindings{
        { RBindingType::UniformBuffer },
        { RBindingType::Texture },
        { RBindingType::Texture },
        { RBindingType::Texture },
        { RBindingType::Texture },
    };

    RBindingGroupLayoutInfo viewportBGLI;
    viewportBGLI.Bindings = bindings.GetView();
    device.CreateBindingGroupLayout(viewportBGL, viewportBGLI);

    return viewportBGL;
}

} // namespace LD