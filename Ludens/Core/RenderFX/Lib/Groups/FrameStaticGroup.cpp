#include "Core/DSA/Include/Array.h"
#include "Core/RenderFX/Include/Groups/FrameStaticGroup.h"

namespace LD
{

FrameStaticGroup::~FrameStaticGroup()
{
    LD_DEBUG_ASSERT(!mDevice);
}

void FrameStaticGroup::Startup(RDevice device, RBindingGroupLayout frameStaticBGL)
{
    LD_DEBUG_ASSERT(device);
    mDevice = device;

    RBufferInfo bufferI;
    bufferI.Type = RBufferType::UniformBuffer;
    bufferI.MemoryUsage = RMemoryUsage::FrameDynamic;
    bufferI.Data = nullptr;
    bufferI.Size = sizeof(FrameStaticLightingUBO);
    mDevice.CreateBuffer(mLightingUBO, bufferI);

    RBindingGroupInfo bgI;
    bgI.Layout = frameStaticBGL;
    mDevice.CreateBindingGroup(mHandle, bgI);

    mHandle.BindUniformBuffer(0, mLightingUBO);
}

void FrameStaticGroup::Cleanup()
{
    mDevice.DeleteBindingGroup(mHandle);
    mDevice.DeleteBuffer(mLightingUBO);
    mDevice.ResetHandle();
}

RBindingGroupLayoutData FrameStaticGroup::GetLayoutData() const
{
    RBindingInfo binding0;
    binding0.Type = RBindingType::UniformBuffer;
    binding0.Count = 1;

    return { binding0 };
}

RBindingGroupLayout FrameStaticGroup::CreateLayout(RDevice device)
{
    LD_DEBUG_ASSERT(device);

    RBindingGroupLayout frameStaticBGL;

    Array<RBindingInfo, 1> bindings{
        { RBindingType::UniformBuffer },
    };

    RBindingGroupLayoutInfo frameStaticBGLI;
    frameStaticBGLI.Bindings = bindings.GetView();
    device.CreateBindingGroupLayout(frameStaticBGL, frameStaticBGLI);

    return frameStaticBGL;
}

} // namespace LD