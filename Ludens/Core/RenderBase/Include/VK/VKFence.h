#pragma once

#include <vulkan/vulkan_core.h>
#include "Core/Header/Include/Error.h"
#include "Core/Header/Include/Types.h"

namespace LD
{

class VKDevice;

class VKFence
{
public:
    VKFence() = default;
    VKFence(const VKFence&) = delete;

    VKFence& operator=(const VKFence&) = delete;

    void Startup(VKDevice& device, const VkFenceCreateInfo& info);
    void Cleanup();

    void Wait(u64 timeout);
    void Reset();

    inline VkFence GetHandle() const
    {
        LD_DEBUG_ASSERT(mHandle != VK_NULL_HANDLE);
        return mHandle;
    }

    explicit operator VkFence() const
    {
        LD_DEBUG_ASSERT(mHandle != VK_NULL_HANDLE);
        return mHandle;
    }

private:
    VkDevice mLogical = VK_NULL_HANDLE;
    VkFence mHandle = VK_NULL_HANDLE;
};

} // namespace LD