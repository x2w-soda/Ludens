#include "Core/RenderBase/Include/VK/VKFence.h"
#include "Core/RenderBase/Include/VK/VKDevice.h"
#include "Core/RenderBase/Include/VK/VKContext.h"

namespace LD
{

void VKFence::Startup(VKDevice& device, const VkFenceCreateInfo& info)
{
    mLogical = device.GetHandle();

    VK_ASSERT(vkCreateFence(mLogical, &info, nullptr, &mHandle));
}

void VKFence::Cleanup()
{
    vkDestroyFence(mLogical, mHandle, nullptr);
}

void VKFence::Wait(u64 timeout)
{
    LD_DEBUG_ASSERT(mHandle != VK_NULL_HANDLE);

    VK_ASSERT(vkWaitForFences(mLogical, 1, &mHandle, VK_TRUE, timeout));
}

void VKFence::Reset()
{
    LD_DEBUG_ASSERT(mHandle != VK_NULL_HANDLE);

    VK_ASSERT(vkResetFences(mLogical, 1, &mHandle));
}

} // namespace LD