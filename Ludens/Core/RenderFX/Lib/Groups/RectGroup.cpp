#include "Core/RenderFX/Include/Groups/RectGroup.h"
#include "Core/DSA/Include/Array.h"

namespace LD
{

RectGroup::~RectGroup()
{
    LD_DEBUG_ASSERT(!mDevice);
}

void RectGroup::Startup(RDevice device, RBindingGroupLayout rectBGL)
{
    LD_DEBUG_ASSERT(device);
    mDevice = device;

    RBindingGroupInfo groupI;
    groupI.Layout = rectBGL;
    mDevice.CreateBindingGroup(mHandle, groupI);
}

void RectGroup::Cleanup()
{
    mDevice.DeleteBindingGroup(mHandle);
    mDevice.ResetHandle();
}

RResult RectGroup::BindTexture(RTexture& textureH, int arrayIndex)
{
    LD_DEBUG_ASSERT(0 <= arrayIndex && arrayIndex < 16);

    return mHandle.BindTexture(0, textureH, arrayIndex);
}

RBindingGroupLayoutData RectGroup::GetLayoutData() const
{
    RBindingInfo binding0;
    binding0.Count = 16;
    binding0.Type = RBindingType::Texture;

    return { binding0 };
}

RBindingGroupLayout RectGroup::CreateLayout(RDevice device)
{
    LD_DEBUG_ASSERT((bool)device);

    RBindingGroupLayout rectBGL;

    RBindingInfo binding;
    binding.Type = RBindingType::Texture;
    binding.Count = 16;

    RBindingGroupLayoutInfo rectBGLI;
    rectBGLI.Bindings = { 1, &binding };
    device.CreateBindingGroupLayout(rectBGL, rectBGLI);

    return rectBGL;
}

} // namespace LD