#include "Core/Header/Include/Error.h"
#include "Core/RenderBase/Include/VK/VKDescriptor.h"
#include "Core/RenderBase/Include/VK/VKBuffer.h"
#include "Core/RenderBase/Include/VK/VKImage.h"
#include "Core/RenderBase/Include/VK/VKDevice.h"
#include "Core/RenderBase/Include/VK/VKContext.h"

namespace LD
{

VKDescriptorPool::VKDescriptorPool()
{
}

VKDescriptorPool::~VKDescriptorPool()
{
    LD_DEBUG_ASSERT(mHandle == VK_NULL_HANDLE);
}

void VKDescriptorPool::Startup(const VKDevice& device, const VkDescriptorPoolCreateInfo& poolCI)
{
    mDevice = device.GetHandle();

    VK_ASSERT(vkCreateDescriptorPool(mDevice, &poolCI, nullptr, &mHandle));
}

void VKDescriptorPool::Cleanup()
{
    vkDestroyDescriptorPool(mDevice, mHandle, nullptr);

    mHandle = VK_NULL_HANDLE;
}

VKDescriptorSetLayout::VKDescriptorSetLayout()
{
}

VKDescriptorSetLayout::~VKDescriptorSetLayout()
{
    LD_DEBUG_ASSERT(mHandle == VK_NULL_HANDLE);
    LD_DEBUG_ASSERT(mDevice == VK_NULL_HANDLE);
}

VKDescriptorSetLayout& VKDescriptorSetLayout::AddBinding(const VkDescriptorSetLayoutBinding& binding)
{
    mBindings.PushBack(binding);
    return *this;
}

void VKDescriptorSetLayout::Startup(const VKDevice& device)
{
    LD_DEBUG_ASSERT(mHandle == VK_NULL_HANDLE && !mBindings.IsEmpty());

    mDevice = device.GetHandle();

    VkDescriptorSetLayoutCreateInfo layoutCI{};
    layoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCI.bindingCount = mBindings.Size();
    layoutCI.pBindings = mBindings.Data();

    VK_ASSERT(vkCreateDescriptorSetLayout(mDevice, &layoutCI, nullptr, &mHandle));
}

void VKDescriptorSetLayout::Cleanup()
{
    vkDestroyDescriptorSetLayout(mDevice, mHandle, nullptr);

    mHandle = VK_NULL_HANDLE;
    mDevice = VK_NULL_HANDLE;
}

VKDescriptorSet::VKDescriptorSet()
{
}

VKDescriptorSet::~VKDescriptorSet()
{
    LD_DEBUG_ASSERT(!mIsAllocated);
}

void VKDescriptorSet::Allocate(VKDevice& device, VKDescriptorPool& pool, VkDescriptorSetLayout layout)
{
    VkDescriptorSetAllocateInfo setAI{};
    setAI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    setAI.descriptorPool = pool.GetHandle();
    setAI.descriptorSetCount = 1;
    setAI.pSetLayouts = &layout;
    vkAllocateDescriptorSets(device.GetHandle(), &setAI, &mHandle);

    mIsAllocated = true;
}

void VKDescriptorSet::Free(VKDevice& device, VKDescriptorPool& pool)
{
    mIsAllocated = false;

    vkFreeDescriptorSets(device.GetHandle(), pool.GetHandle(), 1, &mHandle);
}

void VKDescriptorSet::Write(VKDevice& device, VKBuffer& buffer, u32 binding, u32 index)
{
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = buffer.GetHandle();
    bufferInfo.offset = 0;
    bufferInfo.range = buffer.GetSize();

    VkWriteDescriptorSet setWrite{};
    setWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    setWrite.dstSet = mHandle;
    setWrite.dstBinding = binding;
    setWrite.dstArrayElement = index;
    setWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    setWrite.descriptorCount = 1;
    setWrite.pBufferInfo = &bufferInfo;
    setWrite.pImageInfo = nullptr;
    setWrite.pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(device.GetHandle(), 1, &setWrite, 0, nullptr);
}

void VKDescriptorSet::Write(VKDevice& device, VKSampler& sampler, VKImageView& imageView, u32 binding, u32 index)
{
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.sampler = sampler.GetHandle();
    imageInfo.imageView = imageView.GetHandle();

    VkWriteDescriptorSet setWrite{};
    setWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    setWrite.dstSet = mHandle;
    setWrite.dstBinding = binding;
    setWrite.dstArrayElement = index;
    setWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    setWrite.descriptorCount = 1;
    setWrite.pBufferInfo = nullptr;
    setWrite.pImageInfo = &imageInfo;
    setWrite.pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(device.GetHandle(), 1, &setWrite, 0, nullptr);
}

} // namespace LD