#include "Core/DSA/Include/Array.h"
#include "Core/RenderFX/Include/Groups/MaterialGroup.h"

namespace LD {

MaterialGroup::~MaterialGroup()
{
    LD_DEBUG_ASSERT(!mDevice);
}

void MaterialGroup::Startup(const MaterialGroupInfo& info)
{
    LD_DEBUG_ASSERT(info.Device);
    mDevice = info.Device;
    mMetallicRoughnessLayout = info.MetallicRoughnessLayout;

    RBindingGroupInfo groupI;
    groupI.Layout = info.MaterialBGL;
    mDevice.CreateBindingGroup(mHandle, groupI);

    RBufferInfo uboI{};
    uboI.Type = RBufferType::UniformBuffer;
    uboI.MemoryUsage = RMemoryUsage::Immutable;
    uboI.Data = &info.UBO;
    uboI.Size = sizeof(MaterialUBO);
    mDevice.CreateBuffer(mUBO, uboI);
    mHandle.BindUniformBuffer(0, mUBO);

    // NOTE: We can't conditionally create textures here. Vulkan descripors must reference valid bindings,
    //       if the material does not cotain a specific texture, we create a default texture even if it
    //       won't be accessed in the shader.

    u32 defaultPixel = 0xFF00FFFF;
    RTextureInfo defaultI, textureI;
    defaultI.Type = RTextureType::Texture2D;
    defaultI.Format = RTextureFormat::RGBA8;
    defaultI.Width = 1;
    defaultI.Height = 1;
    defaultI.Data = &defaultPixel;
    defaultI.Size = 4;

    textureI = info.AlbedoTextureInfo.ValueOr(defaultI);
    mDevice.CreateTexture(mAlbedoTexture, textureI);
    mHandle.BindTexture(1, mAlbedoTexture);

    textureI = info.NormalTextureInfo.ValueOr(defaultI);
    mDevice.CreateTexture(mNormalTexture, textureI);
    mHandle.BindTexture(2, mNormalTexture);

    textureI = info.MetallicTextureInfo.ValueOr(defaultI);
    mDevice.CreateTexture(mMetallicTexture, textureI);
    mHandle.BindTexture(3, mMetallicTexture);

    textureI = info.RoughnessTextureInfo.ValueOr(defaultI);
    mDevice.CreateTexture(mRoughnessTexture, textureI);
    mHandle.BindTexture(4, mRoughnessTexture);
}

void MaterialGroup::Cleanup()
{
    mDevice.DeleteTexture(mRoughnessTexture);
    mDevice.DeleteTexture(mMetallicTexture);
    mDevice.DeleteTexture(mNormalTexture);
    mDevice.DeleteTexture(mAlbedoTexture);
    mDevice.DeleteBuffer(mUBO);
    mDevice.DeleteBindingGroup(mHandle);
    mDevice.ResetHandle();
}

RBindingGroupLayoutData MaterialGroup::GetLayoutData() const
{
    RBindingInfo binding0;
    binding0.Count = 1;
    binding0.Type = RBindingType::UniformBuffer;

    RBindingInfo binding1;
    binding1.Count = 1;
    binding1.Type = RBindingType::Texture;

    RBindingInfo binding2 = binding1;
    RBindingInfo binding3 = binding1;
    RBindingInfo binding4 = binding1;

    return { binding0, binding1, binding2, binding3, binding4 };
}

RBindingGroupLayout MaterialGroup::CreateLayout(RDevice device)
{
    LD_DEBUG_ASSERT((bool)device);

    RBindingGroupLayout materialBGL;

    Array<RBindingInfo, 5> bindings{
        { RBindingType::UniformBuffer },
        { RBindingType::Texture }, // albedo
        { RBindingType::Texture }, // normal
        { RBindingType::Texture }, // metallic
        { RBindingType::Texture }, // roughness
    };

    RBindingGroupLayoutInfo materialBGLI;
    materialBGLI.Bindings = bindings.GetView();
    device.CreateBindingGroupLayout(materialBGL, materialBGLI);

    return materialBGL;
}

} // namespace LD