#pragma once

#include <vulkan/vulkan_core.h>
#include "Core/Header/Include/Types.h"
#include "Core/RenderBase/Include/VK/VKMemory.h"

namespace LD
{

class VKContext;
class VKDevice;
class VKCommandPool;

class VKImageView
{
public:
    VKImageView() = default;
    VKImageView(const VKImageView&) = delete;
    ~VKImageView() = default;

    VKImageView& operator=(const VKImageView&) = delete;

    void Startup(const VKDevice& device, const VkImageViewCreateInfo& info);
    void Cleanup();

    inline bool IsValid() const
    {
        return mView != VK_NULL_HANDLE && mHasStartup;
    }
    inline VkImageView GetHandle() const
    {
        return mView;
    }

private:
    bool mHasStartup = false;
    VkDevice mDevice = VK_NULL_HANDLE;
    VkImageView mView = VK_NULL_HANDLE;
};

struct VKImageInfo
{
    VkImageCreateInfo CreateInfo;
    VkMemoryPropertyFlags MemoryProperties;
};

class VKImage
{
public:
    VKImage() = default;
    VKImage(const VKImage&) = delete;
    ~VKImage() = default;

    VKImage& operator=(const VKImage&) = delete;

    void Startup(VKDevice& device, const VKImageInfo& info);
    void Cleanup();

    // use staging buffers to transfer image data to device local memory
    void StageData(u32 layerCount, u32 size, const void** data, VKCommandPool& transferPool, VkQueue transferQueue);

    inline VkImage GetHandle() const
    {
        return mImage;
    }

    inline VKImageInfo GetInfo() const
    {
        return mInfo;
    }

    inline VkFormat GetFormat() const
    {
        return mInfo.CreateInfo.format;
    }
    inline u32 GetWidth() const
    {
        return mInfo.CreateInfo.extent.width;
    }
    inline u32 GetHeight() const
    {
        return mInfo.CreateInfo.extent.height;
    }

private:
    VKImageInfo mInfo;
    VKDevice* mDevice = nullptr;
    VkImage mImage = VK_NULL_HANDLE;
    VKMemory mImageMemory;
};

class VKSampler
{
public:
    VKSampler() = default;
    VKSampler(const VKSampler&) = delete;
    ~VKSampler() = default;

    VKSampler& operator=(const VKSampler&) = delete;

    void Startup(const VKDevice& device, const VkSamplerCreateInfo& info);
    void Cleanup();

    VkSampler GetHandle() const
    {
        return mSampler;
    }

private:
    VkDevice mDevice = VK_NULL_HANDLE;
    VkSampler mSampler = VK_NULL_HANDLE;
};

} // namespace LD