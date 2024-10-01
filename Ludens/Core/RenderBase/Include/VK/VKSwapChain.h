#pragma once

#include <functional>
#include <vulkan/vulkan_core.h>
#include "Core/Header/Include/Types.h"
#include "Core/Header/Include/Observer.h"
#include "Core/OS/Include/Memory.h"
#include "Core/DSA/Include/Vector.h"
#include "Core/RenderBase/Include/VK/VKImage.h"
#include "Core/RenderBase/Include/VK/VKRenderPass.h"
#include "Core/RenderBase/Include/VK/VKFrameBuffer.h"

namespace LD
{

class VKContext;

struct VKSwapChainConfig
{
    VkSurfaceFormatKHR SurfaceFormat;
    VkPresentModeKHR PresentMode;
    VkExtent2D SwapExtent;
    u32 MinImageCount;
};

/// notify observers whenever the swapchain is recreated with a new config
using VKSwapChainInvalidation = VKSwapChainConfig;

class VKSwapChain : public Observable<VKSwapChainInvalidation>
{
public:
    VKSwapChain();
    VKSwapChain(const VKSwapChain&) = delete;
    ~VKSwapChain();

    VKSwapChain& operator=(const VKSwapChain&) = delete;

    using Observer = std::function<void(const VKSwapChain&)>;

    void Startup(const VKContext& context, const VKSwapChainConfig& config);
    void Cleanup();

    inline const VKSwapChainConfig& GetConfig() const
    {
        return mConfig;
    }

    inline const VkExtent2D& GetExtent() const
    {
        return mConfig.SwapExtent;
    }

    inline const VkFormat& GetFormat() const
    {
        return mConfig.SurfaceFormat.format;
    }

    inline const Vector<Ref<VKImageView>>& GetImageViews() const
    {
        return mViews;
    }

    inline VkSwapchainKHR GetHandle() const
    {
        return mHandle;
    }

    // returns the next image index in this swapchain, after waiting on a semaphore
    u32 AcquireImage(VkSemaphore imageAvailable, VkResult& result);

    // presents the swap chain image, after waiting on a semaphore
    void PresentImage(VkSemaphore waitSemaphore, u32 imageIndex);

    void Invalidate(const VKSwapChainConfig& spec);

private:
    void StartupImageViews();
    void CleanupImageViews();

    const VKContext* mContext = nullptr;
    VKSwapChainConfig mConfig;
    VkSwapchainKHR mHandle = VK_NULL_HANDLE;
    VkQueue mPresentQueue = VK_NULL_HANDLE;
    Vector<VkImage> mImages;
    Vector<Ref<VKImageView>> mViews;
};

} // namespace LD