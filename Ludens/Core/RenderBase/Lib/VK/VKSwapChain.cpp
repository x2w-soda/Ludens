#include <iostream>
#include "Core/Header/Include/Error.h"
#include "Core/Header/Include/Types.h"
#include "Core/Application/Include/Application.h"
#include "Core/RenderBase/Include/VK/VKSwapChain.h"
#include "Core/RenderBase/Include/VK/VKContext.h"
#include "Core/RenderBase/Include/VK/VKInfo.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace LD
{

VKSwapChain::VKSwapChain()
{
}

VKSwapChain::~VKSwapChain()
{
    LD_DEBUG_ASSERT(mHandle == VK_NULL_HANDLE);
}

void VKSwapChain::Startup(const VKContext& context, const VKSwapChainConfig& config)
{
    mContext = &context;
    mConfig = config;

    const VKDevice& device = mContext->GetDevice();
    VkSurfaceKHR surface = mContext->GetSurface();
    VKDeviceSurfaceSpec deviceSurfaceSpec = device.GetPhysicalDevice().GetDeviceSurfaceSpec(surface);
    VkDevice logical = device.GetHandle();
    u32 queueFamilyIndices[2] = { device.GetGraphicsIndex(), device.GetPresentIndex() };

    mPresentQueue = device.GetPresentQueue();

    VkSwapchainCreateInfoKHR swapChainCI{};
    swapChainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainCI.surface = surface;
    swapChainCI.minImageCount = mConfig.MinImageCount;
    swapChainCI.imageFormat = mConfig.SurfaceFormat.format;
    swapChainCI.imageColorSpace = mConfig.SurfaceFormat.colorSpace;
    swapChainCI.imageExtent = mConfig.SwapExtent;
    swapChainCI.imageArrayLayers = 1;
    swapChainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // TODO: parameterize
    if (queueFamilyIndices[0] == queueFamilyIndices[1])
    {
        // an image is owned by one queue family at a time
        swapChainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapChainCI.queueFamilyIndexCount = 0;
    }
    else
    {
        // images are used across multiple queue families
        swapChainCI.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapChainCI.queueFamilyIndexCount = 2;
        swapChainCI.pQueueFamilyIndices = queueFamilyIndices;
    }
    swapChainCI.preTransform = deviceSurfaceSpec.SurfaceCapabilities.currentTransform; // TODO: parameterize
    swapChainCI.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapChainCI.presentMode = mConfig.PresentMode;
    swapChainCI.clipped = VK_TRUE;
    swapChainCI.oldSwapchain = VK_NULL_HANDLE;
    VK_ASSERT(vkCreateSwapchainKHR(logical, &swapChainCI, nullptr, &mHandle));

    StartupImageViews();
}

void VKSwapChain::Cleanup()
{
    VkDevice logical = mContext->GetDevice().GetHandle();

    CleanupImageViews();

    vkDestroySwapchainKHR(logical, mHandle, nullptr);

    mHandle = VK_NULL_HANDLE;
}

u32 VKSwapChain::AcquireImage(VkSemaphore imageAvailable, VkResult& result)
{
    VkDevice logical = mContext->GetDevice().GetHandle();
    u32 imageIndex;

    result = vkAcquireNextImageKHR(logical, mHandle, UINT64_MAX, imageAvailable, VK_NULL_HANDLE, &imageIndex);

    if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        // TODO: this should be passed in via new swap chain spec, currently we query just-in-time
        VkExtent2D newExtent;
        Application& app = Application::GetSingleton();
        GLFWwindow* handle = static_cast<GLFWwindow*>(app.GetWindowHandle());
        glfwGetFramebufferSize(handle, (int*)&newExtent.width, (int*)&newExtent.height);

        VKSwapChainConfig newConfig = mConfig;
        newConfig.SwapExtent = newExtent;

        // NOTE: syncrhonous, on-the-spot recreation. caller should still ignore return value and
        //       respect the VkResult from vkAcquireNextImageKHR, swap chain should be valid next frame
        Invalidate(newConfig);
        return 0;
    }
    assert(result == VK_SUCCESS);

    return imageIndex;
}

void VKSwapChain::PresentImage(VkSemaphore waitSemaphore, u32 imageIndex)
{
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &waitSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &mHandle;
    presentInfo.pImageIndices = &imageIndex;

    VK_ASSERT(vkQueuePresentKHR(mPresentQueue, &presentInfo));
}

void VKSwapChain::Invalidate(const VKSwapChainConfig& config)
{
    const VKContext* pContext = mContext;
    const VKDevice& device = mContext->GetDevice();

    vkDeviceWaitIdle(device.GetHandle());

    Cleanup();
    Startup(*pContext, config);

    // the FrameBuffers containing SwapChain image as attachments
    // will have to be recreated
    NotifyObservers(config);
}

void VKSwapChain::StartupImageViews()
{
    const VKDevice& device = mContext->GetDevice();
    VkDevice logical = device.GetHandle();

    // retrieve image handles
    u32 imageCount;
    vkGetSwapchainImagesKHR(logical, mHandle, &imageCount, nullptr);
    mImages.Resize(imageCount);
    vkGetSwapchainImagesKHR(logical, mHandle, &imageCount, mImages.Data());

    // create image views
    mViews.Resize(imageCount);
    VkFormat imageFormat = mConfig.SurfaceFormat.format;
    VkImageAspectFlags imageAspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;

    for (u32 i = 0; i < mViews.Size(); i++)
    {
        VkImageViewCreateInfo imageViewCI =
            VKInfo::ImageViewCreate(VK_IMAGE_VIEW_TYPE_2D, mImages[i], imageFormat, imageAspectFlags);
        mViews[i] = MakeRef<VKImageView>();
        mViews[i]->Startup(device, imageViewCI);
    }
}

void VKSwapChain::CleanupImageViews()
{
    if (mViews.IsEmpty())
        return;

    for (auto& imageView : mViews)
        imageView->Cleanup();
}

} // namespace LD