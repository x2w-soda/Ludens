#pragma once

#include "Core/RenderBase/Include/RBinding.h"
#include "Core/RenderBase/Lib/RBase.h"
#include "Core/RenderBase/Include/VK/VKDescriptor.h"

namespace LD
{

struct RDeviceVK;

struct RBindingGroupLayoutVK : RBindingGroupLayoutBase
{
    RBindingGroupLayoutVK();
    RBindingGroupLayoutVK(const RBindingGroupLayoutVK&) = delete;
    ~RBindingGroupLayoutVK();

    RBindingGroupLayoutVK& operator=(const RBindingGroupLayoutVK&) = delete;

    void Startup(RBindingGroupLayout& layoutH, const RBindingGroupLayoutInfo& info, RDeviceVK& device);
    void Cleanup(RBindingGroupLayout& layoutH);

    VKDescriptorSetLayout DescriptorSetLayout;
};

struct RBindingGroupVK : RBindingGroupBase
{
    RBindingGroupVK();
    RBindingGroupVK(const RBindingGroupVK&) = delete;
    ~RBindingGroupVK();

    RBindingGroupVK& operator=(const RBindingGroupVK&) = delete;

    void Startup(RBindingGroup& groupH, const RBindingGroupInfo& info, RDeviceVK& device);
    void Cleanup(RBindingGroup& groupH);

    virtual RResult BindTexture(u32 binding, RTexture& textureH, int arrayIndex) override;
    virtual RResult BindUniformBuffer(u32 binding, RBuffer& bufferH) override;

    VKDescriptorSet DescriptorSet;
    RDeviceVK* Device = nullptr;
};

} // namespace LD