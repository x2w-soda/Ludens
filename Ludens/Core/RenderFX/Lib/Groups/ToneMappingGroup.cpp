#include "Core/RenderFX/Include/Groups/ToneMappingGroup.h"

namespace LD
{

ToneMappingGroup::~ToneMappingGroup()
{
    LD_DEBUG_ASSERT(!mDevice);
}

void ToneMappingGroup::Startup(RDevice device, RBindingGroupLayout toneMappingBGL)
{
    mDevice = device;

    RBindingGroupInfo groupI;
    groupI.Layout = toneMappingBGL;

    mDevice.CreateBindingGroup(mHandle, groupI);

    RBufferInfo bufferI;
    bufferI.MemoryUsage = RMemoryUsage::FrameDynamic;
    bufferI.Type = RBufferType::UniformBuffer;
    bufferI.Data = nullptr;
    bufferI.Size = sizeof(ToneMappingUBO);
    mDevice.CreateBuffer(mUBO, bufferI);

    mHandle.BindUniformBuffer(0, mUBO);
}

void ToneMappingGroup::Cleanup()
{
    mDevice.DeleteBuffer(mUBO);
    mDevice.DeleteBindingGroup(mHandle);
    mDevice.ResetHandle();
}

RBindingGroupLayoutData ToneMappingGroup::GetLayoutData() const
{
    RBindingInfo binding0;
    binding0.Type = RBindingType::UniformBuffer;
    binding0.Count = 1;

    return { binding0 };
}

RBindingGroupLayout ToneMappingGroup::CreateLayout(RDevice device)
{
    LD_DEBUG_ASSERT((bool)device);

    RBindingGroupLayout toneMappingBGL;

    RBindingInfo binding;
    binding.Type = RBindingType::UniformBuffer;
    binding.Count = 1;

    RBindingGroupLayoutInfo layoutI;
    layoutI.Bindings = { 1, &binding };
    device.CreateBindingGroupLayout(toneMappingBGL, layoutI);

    return toneMappingBGL;
}

} // namespace LD