#pragma once

#include <vulkan/vulkan_core.h>
#include "Core/Header/Include/Error.h"

namespace LD
{

class VKDevice;

class VKSemaphore
{
public:
    VKSemaphore() = default;
    VKSemaphore(const VKSemaphore&) = delete;
    ~VKSemaphore() = default;

    VKSemaphore& operator=(const VKSemaphore&) = delete;

    void Startup(VKDevice& device);
    void Cleanup();

    inline VkSemaphore GetHandle() const
    {
        LD_DEBUG_ASSERT(mHandle != VK_NULL_HANDLE);
        return mHandle;
    }
    explicit operator VkSemaphore() const
    {
        LD_DEBUG_ASSERT(mHandle != VK_NULL_HANDLE);
        return mHandle;
    }

private:
    VkDevice mLogical = VK_NULL_HANDLE;
    VkSemaphore mHandle = VK_NULL_HANDLE;
};

} // namespace LD
