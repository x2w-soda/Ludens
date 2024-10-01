#pragma once

#include <cstdint>
#include <vulkan/vulkan_core.h>
#include "Core/Header/Include/Types.h"
#include "Core/Header/Include/Error.h"
#include "Core/RenderBase/Include/VK/VKMemory.h"
#include "Core/DSA/Include/Vector.h"
#include "Core/DSA/Include/Optional.h"

namespace LD
{

class VKContext;

// a specification of the compatability between the physical device and window surface,
// which is important for creating a suitable swap chain.
struct VKDeviceSurfaceSpec
{
    VkSurfaceCapabilitiesKHR SurfaceCapabilities;
    Vector<VkSurfaceFormatKHR> SurfaceFormats;
    Vector<VkPresentModeKHR> PresentModes;
};

class VKPhysicalDevice
{
public:
    VKPhysicalDevice() : mHandle(VK_NULL_HANDLE){};
    VKPhysicalDevice(VkPhysicalDevice handle, VkSurfaceKHR surface);

    inline const Optional<u32>& GetGraphicsQueueFamily() const
    {
        return mGraphicsIndex;
    }

    inline const Optional<u32>& GetTransferQueueFamily() const
    {
        return mTransferIndex;
    }

    inline const Optional<u32>& GetPresentQueueFamily() const
    {
        return mPresentIndex;
    }

    bool HasExtensionSupport(const Vector<const char*>& desiredExtensionNames) const;
    VKDeviceSurfaceSpec GetDeviceSurfaceSpec(VkSurfaceKHR surface) const;
    VkFormatProperties GetDeviceFormatProperties(VkFormat format) const;

    inline VkPhysicalDevice GetHandle() const
    {
        return mHandle;
    }

    inline VkPhysicalDeviceLimits GetLimits() const
    {
        return mProperties.limits;
    }

    inline VkPhysicalDeviceProperties GetProperties() const
    {
        return mProperties;
    }

    inline VkPhysicalDeviceMemoryProperties GetMemoryProperties() const
    {
        return mMemoryProperties;
    }

    inline VkPhysicalDeviceFeatures GetFeatures() const
    {
        return mFeatures;
    }

    inline const Vector<VkQueueFamilyProperties>& GetQueueFamilyProperties() const
    {
        return mQueueFamilyProperties;
    }

private:
    VkPhysicalDevice mHandle = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties mProperties;
    VkPhysicalDeviceMemoryProperties mMemoryProperties;
    VkPhysicalDeviceFeatures mFeatures;
    Vector<VkExtensionProperties> mExtensionProperties;
    Vector<VkQueueFamilyProperties> mQueueFamilyProperties;
    Optional<u32> mGraphicsIndex; // the index of a queue family capable of graphics ops, if has value
    Optional<u32> mTransferIndex; // the index of a queue family capable of transfer ops, if has value
    Optional<u32> mPresentIndex;  // the index of a queue family capable of present ops, if has value
};

struct VKDeviceSpec
{
    VKPhysicalDevice PhysicalDevice;
};

class VKDevice
{
public:
    VKDevice() = default;
    VKDevice(VkPhysicalDevice, const VKContext&);
    VKDevice(const VKDevice& device) = delete;
    ~VKDevice();

    VKDevice& operator=(const VKDevice& rhs) = delete;

    void Startup(const VKContext& context, const VKDeviceSpec& spec);
    void Cleanup();

    inline bool IsValid() const
    {
        return mHandle != VK_NULL_HANDLE;
    }

    inline VkDevice GetHandle() const
    {
        return mHandle;
    }

    inline const VKPhysicalDevice& GetPhysicalDevice() const
    {
        return mPhysical;
    }

    inline u32 GetGraphicsIndex() const
    {
        return mPhysical.GetGraphicsQueueFamily().Value();
    }

    inline u32 GetTransferIndex() const
    {
        return mPhysical.GetTransferQueueFamily().Value();
    }

    inline u32 GetPresentIndex() const
    {
        return mPhysical.GetPresentQueueFamily().Value();
    }

    inline VkQueue GetGraphicsQueue() const
    {
        return mGraphicsQueue;
    }

    inline VkQueue GetTransferQueue() const
    {
        return mTransferQueue;
    }

    inline VkQueue GetPresentQueue() const
    {
        return mPresentQueue;
    }

    // returns true and writes to typeIndex if we find a memory type that satisfies the given requirements
    bool GetMemoryType(u32 typeFilter, VkMemoryPropertyFlags typeFlags, u32* typeIndex);

private:
    VKPhysicalDevice mPhysical{};
    VkDevice mHandle = VK_NULL_HANDLE;
    VkQueue mGraphicsQueue;
    VkQueue mTransferQueue;
    VkQueue mPresentQueue;
};

} // namespace LD