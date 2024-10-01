#include "Core/RenderBase/Include/VK/VKInfo.h"
#include "Core/RenderBase/Lib/RBindingVK.h"
#include "Core/RenderBase/Lib/RDeviceVK.h"
#include "Core/RenderBase/Lib/RDeriveVK.h"

namespace LD
{

RBindingGroupLayoutVK::RBindingGroupLayoutVK()
{
}

RBindingGroupLayoutVK::~RBindingGroupLayoutVK()
{
    LD_DEBUG_ASSERT(ID == 0);
}

void RBindingGroupLayoutVK::Startup(RBindingGroupLayout& layoutH, const RBindingGroupLayoutInfo& info,
                                    RDeviceVK& device)
{
    RBindingGroupLayoutBase::Startup(layoutH, info, (RDeviceBase*)&device);
    VKContext& context = device.Context;

    for (size_t bindingIdx = 0; bindingIdx < info.Bindings.Size(); bindingIdx++)
    {
        const RBindingInfo binding = info.Bindings[bindingIdx];
        VkDescriptorSetLayoutBinding setLayoutBinding =
            VKInfo::DescriptorSetLayoutBinding(bindingIdx, DeriveVKDescriptorType(binding.Type), binding.Count,
                                               VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
        DescriptorSetLayout.AddBinding(setLayoutBinding);
    }

    DescriptorSetLayout.Startup(context.GetDevice());
}

void RBindingGroupLayoutVK::Cleanup(RBindingGroupLayout& layoutH)
{
    RBindingGroupLayoutBase::Cleanup(layoutH);

    DescriptorSetLayout.Cleanup();
}

RBindingGroupVK::RBindingGroupVK()
{
}

RBindingGroupVK::~RBindingGroupVK()
{
    LD_DEBUG_ASSERT(ID == 0);
}

void RBindingGroupVK::Startup(RBindingGroup& groupH, const RBindingGroupInfo& info, RDeviceVK& device)
{
    RBindingGroupBase::Startup(groupH, info, (RDeviceBase*)&device);
    RBindingGroupLayoutVK& layout = Derive<RBindingGroupLayoutVK>(info.Layout);
    VKContext& vkContext = device.Context;

    Device = &device;
    DescriptorSet.Allocate(vkContext.GetDevice(), device.DescriptorPool, layout.DescriptorSetLayout.GetHandle());
}

void RBindingGroupVK::Cleanup(RBindingGroup& groupH)
{
    RBindingGroupBase::Cleanup(groupH);
    VKContext& vkContext = Device->Context;

    DescriptorSet.Free(vkContext.GetDevice(), Device->DescriptorPool);
    Device = nullptr;
}

RResult RBindingGroupVK::BindTexture(u32 binding, RTexture& textureH, int arrayIndex)
{
    VKContext& vkContext = Device->Context;
    VKSampler& vkSampler = Derive<RTextureVK>(textureH).Sampler;
    Ref<VKImageView>& vkImageView = Derive<RTextureVK>(textureH).ImageView;

    LD_DEBUG_ASSERT(vkSampler.GetHandle() != VK_NULL_HANDLE);
    LD_DEBUG_ASSERT(vkImageView && vkImageView->GetHandle() != VK_NULL_HANDLE);

    // NOTE: direct call to vkUpdateDescriptorSets with single write,
    //       resources must not be currently accessed by GPU
    DescriptorSet.Write(vkContext.GetDevice(), vkSampler, *vkImageView, binding, arrayIndex);

    return {};
}

RResult RBindingGroupVK::BindUniformBuffer(u32 binding, RBuffer& bufferH)
{
    VKContext& vkContext = Device->Context;
    VKBuffer& vkBuffer = Derive<RBufferVK>(bufferH).Buffer;

    // NOTE: direct call to vkUpdateDescriptorSets with single write,
    //       resources must not be currently accessed by GPU
    DescriptorSet.Write(vkContext.GetDevice(), vkBuffer, binding, 0);

    return {};
}

} // namespace LD