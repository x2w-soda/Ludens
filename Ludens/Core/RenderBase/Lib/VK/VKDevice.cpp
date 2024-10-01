#include <iostream>
#include <cstring>
#include <set>
#include <vulkan/vulkan_core.h>
#include "Core/Header/Include/Types.h"
#include "Core/Header/Include/Error.h"
#include "Core/RenderBase/Include/VK/VKContext.h"
#include "Core/RenderBase/Include/VK/VKDevice.h"
#include "Core/RenderBase/Include/VK/VKMemory.h"

namespace LD
{

VKPhysicalDevice::VKPhysicalDevice(VkPhysicalDevice handle, VkSurfaceKHR surface) : mHandle(handle)
{
    vkGetPhysicalDeviceProperties(mHandle, &mProperties);
    vkGetPhysicalDeviceMemoryProperties(mHandle, &mMemoryProperties);
    vkGetPhysicalDeviceFeatures(mHandle, &mFeatures);

    u32 extensionCount;
    VK_ASSERT(vkEnumerateDeviceExtensionProperties(mHandle, nullptr, &extensionCount, nullptr));
    mExtensionProperties.Resize(extensionCount);
    VK_ASSERT(vkEnumerateDeviceExtensionProperties(mHandle, nullptr, &extensionCount, mExtensionProperties.Data()));

    u32 queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(mHandle, &queueFamilyCount, nullptr);
    mQueueFamilyProperties.Resize(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(mHandle, &queueFamilyCount, mQueueFamilyProperties.Data());

    for (u32 i = 0; i < mQueueFamilyProperties.Size(); i++)
    {
        const auto& property = mQueueFamilyProperties[i];

        // select first queue family that supports graphics operations
        if (!mGraphicsIndex.HasValue() && (property.queueFlags & VK_QUEUE_GRAPHICS_BIT))
            mGraphicsIndex = i;

        // select first queue family that supports graphics operations
        if (!mTransferIndex.HasValue() && (property.queueFlags & VK_QUEUE_TRANSFER_BIT))
            mTransferIndex = i;

        // select first queue family that supports presentation
        VkBool32 hasPresentSupport;
        vkGetPhysicalDeviceSurfaceSupportKHR(mHandle, i, surface, &hasPresentSupport);
        if (!mPresentIndex.HasValue() && hasPresentSupport)
            mPresentIndex = i;
    }
}

bool VKPhysicalDevice::HasExtensionSupport(const Vector<const char*>& desiredExtensionNames) const
{
    for (const char* extensionName : desiredExtensionNames)
    {
        bool found = false;

        std::cout << "Device Extension [" << extensionName;

        for (const VkExtensionProperties& properties : mExtensionProperties)
            if (strncmp(properties.extensionName, extensionName, strlen(extensionName)) == 0)
            {
                found = true;
                std::cout << "] Found\n";
                break;
            }

        if (!found)
        {
            std::cout << "] Not found, aborting\n";
            return false;
        }
    }

    return true;
}

VKDeviceSurfaceSpec VKPhysicalDevice::GetDeviceSurfaceSpec(VkSurfaceKHR surface) const
{
    VKDeviceSurfaceSpec spec{};
    u32 formatCount, presentModeCount;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mHandle, surface, &spec.SurfaceCapabilities);

    vkGetPhysicalDeviceSurfaceFormatsKHR(mHandle, surface, &formatCount, nullptr);
    if (formatCount > 0)
    {
        spec.SurfaceFormats.Resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(mHandle, surface, &formatCount, spec.SurfaceFormats.Data());
    }

    vkGetPhysicalDeviceSurfacePresentModesKHR(mHandle, surface, &presentModeCount, nullptr);
    if (presentModeCount > 0)
    {
        spec.PresentModes.Resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(mHandle, surface, &presentModeCount, spec.PresentModes.Data());
    }

    return spec;
}

VkFormatProperties VKPhysicalDevice::GetDeviceFormatProperties(VkFormat format) const
{
    LD_DEBUG_ASSERT(mHandle != VK_NULL_HANDLE);

    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(mHandle, format, &props);

    return props;
}

VKDevice::~VKDevice()
{
    LD_DEBUG_ASSERT(mHandle == VK_NULL_HANDLE);
}

void VKDevice::Startup(const VKContext& context, const VKDeviceSpec& spec)
{
    LD_DEBUG_ASSERT(spec.PhysicalDevice.GetHandle() != VK_NULL_HANDLE && "missing physical device");

    // NOTE: potentially large copy assignment
    mPhysical = spec.PhysicalDevice;

    LD_DEBUG_ASSERT(mPhysical.GetGraphicsQueueFamily().HasValue());
    LD_DEBUG_ASSERT(mPhysical.GetTransferQueueFamily().HasValue());
    LD_DEBUG_ASSERT(mPhysical.GetPresentQueueFamily().HasValue());

    VkPhysicalDeviceFeatures enabledFeatures{};
    Vector<const char*> desiredExtensionNames;
    Vector<const char*> desiredLayerNames;
    float queuePriority = 1.0f;

    enabledFeatures.samplerAnisotropy = VK_TRUE;

    // create one device queue from each unique queue family
    Vector<VkDeviceQueueCreateInfo> deviceQueueCIs{};
    std::set<u32> uniqueQueueFamilyIndices{
        mPhysical.GetGraphicsQueueFamily().Value(),
        mPhysical.GetTransferQueueFamily().Value(),
        mPhysical.GetPresentQueueFamily().Value(),
    };

    for (u32 queueFamilyIndex : uniqueQueueFamilyIndices)
    {
        std::cout << "creating 1 device queue from queue family " << queueFamilyIndex << std::endl;

        VkDeviceQueueCreateInfo deviceQueueCI{};
        deviceQueueCI.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        deviceQueueCI.queueFamilyIndex = queueFamilyIndex;
        deviceQueueCI.queueCount = 1;
        deviceQueueCI.pQueuePriorities = &queuePriority;
        deviceQueueCIs.PushBack(deviceQueueCI);
    }

    desiredExtensionNames = context.GetDesiredExtensions();

    VkDeviceCreateInfo deviceCI{};
    deviceCI.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCI.queueCreateInfoCount = deviceQueueCIs.Size();
    deviceCI.pQueueCreateInfos = deviceQueueCIs.Data();
    deviceCI.pEnabledFeatures = &enabledFeatures;
    deviceCI.enabledExtensionCount = desiredExtensionNames.Size();
    deviceCI.ppEnabledExtensionNames = desiredExtensionNames.Data();
    if (context.HasValidationSupport())
    {
        desiredLayerNames = context.GetDesiredLayers();
        deviceCI.enabledLayerCount = desiredLayerNames.Size();
        deviceCI.ppEnabledLayerNames = desiredLayerNames.Data();
    }
    else
        deviceCI.enabledLayerCount = 0;

    VK_ASSERT(vkCreateDevice(mPhysical.GetHandle(), &deviceCI, nullptr, &mHandle));

    // retrieve handles
    vkGetDeviceQueue(mHandle, mPhysical.GetGraphicsQueueFamily().Value(), 0, &mGraphicsQueue);
    vkGetDeviceQueue(mHandle, mPhysical.GetTransferQueueFamily().Value(), 0, &mTransferQueue);
    vkGetDeviceQueue(mHandle, mPhysical.GetPresentQueueFamily().Value(), 0, &mPresentQueue);

    std::cout << "VKDevice setup complete" << std::endl;
}

void VKDevice::Cleanup()
{
    vkDestroyDevice(mHandle, nullptr);
    mHandle = VK_NULL_HANDLE;

    std::cout << "VKDevice cleanup complete" << std::endl;
}

bool VKDevice::GetMemoryType(u32 typeFilter, VkMemoryPropertyFlags typeFlags, u32* typeIndex)
{
    const VkPhysicalDeviceMemoryProperties& props = mPhysical.GetMemoryProperties();

    for (u32 i = 0; i < props.memoryTypeCount; i++)
    {
        const VkMemoryType& memoryType = props.memoryTypes[i];

        if (typeFilter & (1 << i) && (typeFlags & memoryType.propertyFlags))
        {
            *typeIndex = i;
            return true;
        }
    }

    return false;
}

} // namespace LD