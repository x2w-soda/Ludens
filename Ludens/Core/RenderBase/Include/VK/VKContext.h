#pragma once

#include <cassert>
#include <vulkan/vulkan_core.h>
#include "Core/RenderBase/Include/VK/VKDevice.h"
#include "Core/RenderBase/Include/VK/VKSwapChain.h"
#include "Core/DSA/Include/Vector.h"

#define VK_ASSERT(VK_RESULT)                                                                                           \
    {                                                                                                                  \
        VkResult result = VK_RESULT;                                                                                   \
        assert(result == VK_SUCCESS);                                                                                  \
    }

namespace LD
{

struct VKContextInfo
{
};

/// Bundles the Vulkan Instance, Surface, Swapchain, and a logical Device
class VKContext
{
public:
    VKContext() = default;
    VKContext(const VKContext&) = delete;
    ~VKContext() = default;

    VKContext& operator=(const VKContext&) = delete;

    void Startup(const VKContextInfo& config);
    void Cleanup();

    inline bool HasStartup() const
    {
        return mHasStartup;
    }

    inline bool HasValidationSupport() const
    {
        return mHasValidationSupport;
    }

    inline Vector<const char*> GetDesiredLayers() const
    {
        return mDesiredLayerNames;
    }

    inline Vector<const char*> GetDesiredExtensions() const
    {
        return mDesiredExtensionNames;
    }

    inline const VkInstance& GetInstance() const
    {
        return mInstance;
    }

    inline const VkSurfaceKHR& GetSurface() const
    {
        return mSurface;
    }

    inline const VkFormat GetSurfaceFormat() const
    {
        return mSwapChain.GetFormat();
    }

    inline const VkFormat GetDepthStencilFormat() const
    {
        return mDepthStencilFormat;
    }

    inline const VKDevice& GetDevice() const
    {
        return mDevice;
    }

    inline const VKSwapChain& GetSwapChain() const
    {
        return mSwapChain;
    }

    inline VkInstance& GetInstance()
    {
        return mInstance;
    }

    inline VkSurfaceKHR& GetSurface()
    {
        return mSurface;
    }

    inline VKDevice& GetDevice()
    {
        return mDevice;
    }

    inline VKSwapChain& GetSwapChain()
    {
        return mSwapChain;
    }

private:
    void StartupInstance();
    void StartupSurface();
    void StartupDevice();
    void StartupSwapChain();
    void CleanupInstance();
    void CleanupSurface();
    void CleanupDevice();
    void CleanupSwapChain();
    bool CheckDepthStencilFormatSupport();
    bool CheckValidationSupport();
    bool CheckPhysicalDeviceSupport(const VKPhysicalDevice& physical);

    bool mHasStartup = false;
    VkInstance mInstance;
    VkSurfaceKHR mSurface;
    VKDevice mDevice;
    VKSwapChain mSwapChain;
    VkFormat mDepthStencilFormat;
    VkImageTiling mDepthStencilImageTiling;

    // device extensions and validation layers
    uint32_t mExtensionCount;
    uint32_t mLayerCount;
    Vector<VkExtensionProperties> mExtensions;
    Vector<VkLayerProperties> mLayers;
    Vector<const char*> mDesiredExtensionNames;
    Vector<const char*> mDesiredLayerNames;
    bool mHasValidationSupport;
};

} // namespace LD