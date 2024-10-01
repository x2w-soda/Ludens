#include "Core/RenderFX/Include/Groups/CubemapGroup.h"
#include "Core/DSA/Include/Array.h"

namespace LD
{

void CubemapGroup::Startup(RDevice device, RBindingGroupLayout cubemapBGL, RTexture cubemap)
{
    LD_DEBUG_ASSERT(cubemap);
    mDevice = device;

    RBindingGroupInfo bgI;
    bgI.Layout = cubemapBGL;
    mDevice.CreateBindingGroup(mHandle, bgI);
    mHandle.BindTexture(0, cubemap);
}

void CubemapGroup::Cleanup()
{
    mDevice.DeleteBindingGroup(mHandle);
}

RBindingGroupLayoutData CubemapGroup::GetLayoutData() const
{
    RBindingInfo cubemap;
    cubemap.Count = 1;
    cubemap.Type = RBindingType::Texture;

    return { cubemap };
}

RBindingGroupLayout CubemapGroup::CreateLayout(RDevice device)
{
    RBindingGroupLayout cubemapBGL;

    Array<RBindingInfo, 1> bindings;
    bindings[0] = { RBindingType::Texture };

    RBindingGroupLayoutInfo cubemapBGLI;
    cubemapBGLI.Bindings = bindings.GetView();
    device.CreateBindingGroupLayout(cubemapBGL, cubemapBGLI);

    return cubemapBGL;
}

} // namespace LD