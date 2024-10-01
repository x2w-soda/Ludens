#include "Core/RenderBase/Include/VK/VKContext.h"
#include "Core/RenderBase/Include/VK/VKMemory.h"
#include "Core/RenderBase/Include/VK/VKDevice.h"

namespace LD
{

void VKMemory::Allocate(VKDevice& device, const VKMemorySpec& spec)
{
    mSpec = spec;

    VkMemoryAllocateInfo memoryAI{};
    memoryAI.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAI.allocationSize = spec.Size;
    memoryAI.memoryTypeIndex = spec.MemoryType;

    VK_ASSERT(vkAllocateMemory(device.GetHandle(), &memoryAI, nullptr, &mHandle));
}

void VKMemory::Free(VKDevice& device)
{
    LD_DEBUG_ASSERT(mHandle != VK_NULL_HANDLE);

    vkFreeMemory(device.GetHandle(), mHandle, nullptr);
}

void* VKMemory::Map(VKDevice& device)
{
    LD_DEBUG_ASSERT(mHandle != VK_NULL_HANDLE);

    void* mem;
    VK_ASSERT(vkMapMemory(device.GetHandle(), mHandle, 0, mSpec.Size, 0, &mem));
    return mem;
}

void VKMemory::Unmap(VKDevice& device)
{
    LD_DEBUG_ASSERT(mHandle != VK_NULL_HANDLE);

    vkUnmapMemory(device.GetHandle(), mHandle);
}

} // namespace LD