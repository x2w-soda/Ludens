#include "Core/RenderBase/Include/VK/VKSemaphore.h"
#include "Core/RenderBase/Include/VK/VKDevice.h"
#include "Core/RenderBase/Include/VK/VKContext.h"

namespace LD
{

void VKSemaphore::Startup(VKDevice& device)
{
    mLogical = device.GetHandle();

    VkSemaphoreCreateInfo semaphoreCI{};
    semaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VK_ASSERT(vkCreateSemaphore(mLogical, &semaphoreCI, nullptr, &mHandle));
}

void VKSemaphore::Cleanup()
{
    vkDestroySemaphore(mLogical, mHandle, nullptr);
}

} // namespace LD