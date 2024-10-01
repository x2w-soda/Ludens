#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <algorithm>
#include "Core/Application/Include/Application.h"
#include "Core/RenderBase/Include/VK/VKContext.h"
#include "Core/RenderBase/Include/VK/VKInfo.h"
#include "Core/Header/Include/Error.h"
#include "Core/DSA/Include/Array.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define CLAMP(X, MIN, MAX) (((X) < (MIN)) ? (MIN) : (((X) > (MAX)) ? (MAX) : (X)))

namespace LD
{

void VKContext::Startup(const VKContextInfo& info)
{
    LD_DEBUG_ASSERT(!mHasStartup);

    StartupInstance();
    StartupSurface();
    StartupDevice();
    StartupSwapChain();

    bool found = CheckDepthStencilFormatSupport();
    LD_DEBUG_ASSERT(found && "none of the candidate depth stencil formats are supported");

    std::cout << "VKContext Startup complete" << std::endl;
    mHasStartup = true;
}

void VKContext::Cleanup()
{
    LD_DEBUG_ASSERT(mHasStartup);

    mHasStartup = false;

    CleanupSwapChain();
    CleanupDevice();
    CleanupSurface();
    CleanupInstance();

    std::cout << "VKContext Cleanup complete" << std::endl;
}

void VKContext::StartupInstance()
{
    // here are the desired device extensions from the backend
    mDesiredExtensionNames.Clear();
    mDesiredExtensionNames.PushBack(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    mDesiredExtensionNames.PushBack(VK_KHR_MAINTENANCE1_EXTENSION_NAME);

    // here are the desired validation layers from the backend
    mDesiredLayerNames.Clear();
    mDesiredLayerNames.PushBack("VK_LAYER_KHRONOS_validation");

    vkEnumerateInstanceExtensionProperties(nullptr, &mExtensionCount, nullptr);
    mExtensions.Resize(mExtensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &mExtensionCount, mExtensions.Data());

    vkEnumerateInstanceLayerProperties(&mLayerCount, nullptr);
    mLayers.Resize(mLayerCount);
    vkEnumerateInstanceLayerProperties(&mLayerCount, mLayers.Data());

    // validation layers
    // TODO: setup validation layer message callback
    mHasValidationSupport = CheckValidationSupport();
    if (!mHasValidationSupport)
    {
        std::cout << "validation layers not supported" << std::endl;
    }

    std::cout << "VKContext vkInstance properties (" << mExtensionCount << ")" << std::endl;
    for (const auto& properties : mExtensions)
    {
        std::cout << '\t' << properties.extensionName << " version " << properties.specVersion << std::endl;
    }

    VkApplicationInfo appI{};
    appI.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appI.pApplicationName = "Hello Triangle"; // TODO:
    appI.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
    appI.pEngineName = "No Engine";
    appI.engineVersion = VK_MAKE_VERSION(0, 0, 1);
    appI.apiVersion = VK_API_VERSION_1_0;

    // GLFW may require some instance extensions
    u32 glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::cout << "VKContext glfw requried instance extensions (" << glfwExtensionCount << ")" << std::endl;
    for (u32 i = 0; i < glfwExtensionCount; i++)
    {
        std::cout << '\t' << glfwExtensions[i] << std::endl;
    }

    VkInstanceCreateInfo instanceCI{};
    instanceCI.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCI.pApplicationInfo = &appI;
    instanceCI.enabledExtensionCount = glfwExtensionCount;
    instanceCI.ppEnabledExtensionNames = glfwExtensions;
    if (mHasValidationSupport)
    {
        instanceCI.enabledLayerCount = mDesiredLayerNames.Size();
        instanceCI.ppEnabledLayerNames = mDesiredLayerNames.Data();
    }
    else
        instanceCI.enabledLayerCount = 0;

    VK_ASSERT(vkCreateInstance(&instanceCI, nullptr, &mInstance));
}

void VKContext::StartupSurface()
{
    Application& app = Application::GetSingleton();
    GLFWwindow* handle = static_cast<GLFWwindow*>(app.GetWindowHandle());

    // TODO: abstract raw glfw calls, should call from ApplicationWindow
    VK_ASSERT(glfwCreateWindowSurface(mInstance, handle, nullptr, &mSurface));
}

void VKContext::StartupDevice()
{
    u32 physicalDeviceCount;
    Vector<VkPhysicalDevice> physicalDevices;
    Optional<VKPhysicalDevice> selectedDevice;

    // enumerate all GPUs and use the first one that supports all required criteria

    vkEnumeratePhysicalDevices(mInstance, &physicalDeviceCount, nullptr);
    if (physicalDeviceCount == 0)
    {
        std::cout << "No GPUs found" << std::endl;
        return;
    }
    physicalDevices.Resize(physicalDeviceCount);
    vkEnumeratePhysicalDevices(mInstance, &physicalDeviceCount, physicalDevices.Data());

    bool foundPhysicalDevice = false;
    for (auto& physicalDevice : physicalDevices)
    {
        VKPhysicalDevice candidateDevice(physicalDevice, GetSurface());

        if (CheckPhysicalDeviceSupport(candidateDevice))
        {
            selectedDevice = candidateDevice;

            VkPhysicalDeviceProperties properties = candidateDevice.GetProperties();
            VkPhysicalDeviceLimits& limits = properties.limits;
            std::cout << "Selected GPU: " << properties.deviceName << std::endl;
            std::cout << "- Push Constant Size: " << limits.maxPushConstantsSize << std::endl;
            std::cout << "- Max Bound Descriptor Sets: " << limits.maxBoundDescriptorSets << std::endl;
            break;
        }
    }

    if (!selectedDevice.HasValue())
    {
        std::cout << "No suitable GPUs found" << std::endl;
        return;
    }

    VKDeviceSpec config{};
    config.PhysicalDevice = selectedDevice.Value();
    mDevice.Startup(*this, config);

    if (!mDevice.IsValid())
    {
        std::cout << "Failed to setup GPU" << std::endl;
        return;
    }
}

void VKContext::StartupSwapChain()
{
    const VKPhysicalDevice& physical = mDevice.GetPhysicalDevice();
    VKDeviceSurfaceSpec deviceSurfaceSpec = physical.GetDeviceSurfaceSpec(mSurface);
    VKSwapChainConfig swapChainSpec{};

    // choose a surface format, looks for VK_FORMAT_B8G8R8A8_UNORM and
    // defaults to the first available format otherwise.
    {
        swapChainSpec.SurfaceFormat = deviceSurfaceSpec.SurfaceFormats[0];

        for (const VkSurfaceFormatKHR& surfaceFormat : deviceSurfaceSpec.SurfaceFormats)
        {
            if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
                surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                swapChainSpec.SurfaceFormat = surfaceFormat;
                break;
            }
        }
    }

    // choose a present mode
    {
        swapChainSpec.PresentMode = VK_PRESENT_MODE_FIFO_KHR; // guaranteed support

        for (VkPresentModeKHR mode : deviceSurfaceSpec.PresentModes)
        {
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
                swapChainSpec.PresentMode = mode;
        }

        std::cout << "Presentation Mode: " << swapChainSpec.PresentMode << std::endl;
    }

    // choose a swap extent
    {
        Application& app = Application::GetSingleton();
        GLFWwindow* handle = static_cast<GLFWwindow*>(app.GetWindowHandle());
        const VkSurfaceCapabilitiesKHR& caps = deviceSurfaceSpec.SurfaceCapabilities;

        if (caps.currentExtent.width != std::numeric_limits<u32>::max())
            swapChainSpec.SwapExtent = caps.currentExtent;
        else
        {
            int width, height;
            glfwGetFramebufferSize(handle, &width, &height);

            VkExtent2D extent = { static_cast<u32>(width), static_cast<u32>(height) };
            extent.width = CLAMP(extent.width, caps.minImageExtent.width, caps.maxImageExtent.width);
            extent.height = CLAMP(extent.height, caps.minImageExtent.height, caps.maxImageExtent.height);
            swapChainSpec.SwapExtent = extent;
        }
    }

    // choose minimum number of images in swapchain,
    // here we choose the driver minimum plus one
    {
        swapChainSpec.MinImageCount = deviceSurfaceSpec.SurfaceCapabilities.minImageCount + 1;
    }

    mSwapChain.Startup(*this, swapChainSpec);
}

void VKContext::CleanupInstance()
{
    vkDestroyInstance(mInstance, nullptr);
}

void VKContext::CleanupSurface()
{
    vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
}

void VKContext::CleanupDevice()
{
    mDevice.Cleanup();
}

void VKContext::CleanupSwapChain()
{
    mSwapChain.Cleanup();
}

bool VKContext::CheckDepthStencilFormatSupport()
{
    mDevice.GetHandle();

    Array<VkFormat, 3> candidates{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };

    mDepthStencilImageTiling = VK_IMAGE_TILING_OPTIMAL;
    VkFormatFeatureFlags features = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;

    for (uint32_t i = 0; i < candidates.Size(); i++)
    {
        VkFormat format = candidates[i];
        const VkFormatProperties& formatProps = mDevice.GetPhysicalDevice().GetDeviceFormatProperties(format);
        VkFormatFeatureFlags linearTilingFlags = formatProps.linearTilingFeatures;
        VkFormatFeatureFlags optimalTilingFlags = formatProps.optimalTilingFeatures;

        if (mDepthStencilImageTiling == VK_IMAGE_TILING_LINEAR && (linearTilingFlags & features) == features)
        {
            mDepthStencilFormat = format;
            return true;
        }

        if (mDepthStencilImageTiling == VK_IMAGE_TILING_OPTIMAL && (optimalTilingFlags & features) == features)
        {
            mDepthStencilFormat = format;
            return true;
        }
    }

    return false;
}

bool VKContext::CheckValidationSupport()
{
    for (const char* layerName : mDesiredLayerNames)
    {
        bool found = false;

        std::cout << "Validation Layer [" << layerName;

        for (const VkLayerProperties& properties : mLayers)
            if (strncmp(properties.layerName, layerName, strlen(layerName)) == 0)
            {
                found = true;
                std::cout << "] found\n";
                break;
            }

        if (!found)
        {
            std::cout << "] not found, aborting\n";
            return false;
        }
    }

    return true;
}

bool VKContext::CheckPhysicalDeviceSupport(const VKPhysicalDevice& physical)
{
    VkPhysicalDeviceProperties properties = physical.GetProperties();
    VkPhysicalDeviceFeatures features = physical.GetFeatures();
    VKDeviceSurfaceSpec surfaceSpec = physical.GetDeviceSurfaceSpec(mSurface);

    bool isValidGPU = properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ||
                      properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
    if (!isValidGPU)
        return false;

    bool hasFeatureSupport = features.geometryShader && features.samplerAnisotropy;
    if (!hasFeatureSupport)
        return false;

    bool hasExtensionSupport = physical.HasExtensionSupport(mDesiredExtensionNames);
    if (!hasExtensionSupport)
        return false;

    bool hasQueueSupport = physical.GetGraphicsQueueFamily().HasValue() &&
                           physical.GetTransferQueueFamily().HasValue() && physical.GetPresentQueueFamily().HasValue();
    if (!hasQueueSupport)
        return false;

    // NOTE: this criteria should only be checked after confirming hasExtensionSupport
    bool hasSwapChainSupport = !surfaceSpec.SurfaceFormats.IsEmpty() && !surfaceSpec.PresentModes.IsEmpty();
    if (!hasSwapChainSupport)
        return false;

    // the GPU has passed the interview
    return true;
}

} // namespace LD