#pragma once

#include <vulkan/vulkan_core.h>
#include "Core/Header/Include/Types.h"
#include "Core/DSA/Include/Vector.h"

namespace LD
{

class VKDevice;
class VKBuffer;
class VKImage;
class VKImageView;
class VKSampler;

class VKDescriptorPool
{
public:
    VKDescriptorPool();
    VKDescriptorPool(const VKDescriptorPool&) = delete;
    ~VKDescriptorPool();

    VKDescriptorPool& operator=(const VKDescriptorPool&) = delete;

    void Startup(const VKDevice& device, const VkDescriptorPoolCreateInfo& poolCI);
    void Cleanup();

    inline VkDescriptorPool GetHandle()
    {
        return mHandle;
    }

private:
    VkDescriptorPoolCreateFlags mFlags;
    VkDescriptorPool mHandle = VK_NULL_HANDLE;
    VkDevice mDevice = VK_NULL_HANDLE;
    Vector<VkDescriptorPoolSize> mSizes;
    u32 mMaxSets = 0;
};

class VKDescriptorSetLayout
{
public:
    VKDescriptorSetLayout();
    VKDescriptorSetLayout(const VKDescriptorSetLayout&) = delete;
    ~VKDescriptorSetLayout();

    VKDescriptorSetLayout& operator=(const VKDescriptorSetLayout&) = delete;

    VKDescriptorSetLayout& AddBinding(const VkDescriptorSetLayoutBinding& binding);

    void Startup(const VKDevice& device);
    void Cleanup();

    inline VkDescriptorSetLayout GetHandle() const
    {
        return mHandle;
    }

private:
    VkDevice mDevice = VK_NULL_HANDLE;
    VkDescriptorSetLayout mHandle = VK_NULL_HANDLE;
    Vector<VkDescriptorSetLayoutBinding> mBindings;
};

class VKDescriptorSet
{
public:
    VKDescriptorSet();
    VKDescriptorSet(const VKDescriptorSet&) = delete;
    ~VKDescriptorSet();

    VKDescriptorSet& operator=(const VKDescriptorSet&) = delete;

    inline VkDescriptorSet GetHandle() const
    {
        return mHandle;
    }

    void Allocate(VKDevice& device, VKDescriptorPool& pool, VkDescriptorSetLayout layout);
    void Free(VKDevice& device, VKDescriptorPool& pool);

    /// @brief single set write for buffer
    void Write(VKDevice& device, VKBuffer& buffer, u32 binding, u32 index);

    /// @brief single set write for image sampler
    void Write(VKDevice& device, VKSampler& sampler, VKImageView& imageView, u32 binding, u32 index);

private:
    VkDescriptorSet mHandle = VK_NULL_HANDLE;
    bool mIsAllocated = false;
};

} // namespace LD