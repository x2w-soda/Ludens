#pragma once

#include <vulkan/vulkan_core.h>
#include "Core/Header/Include/Types.h"

namespace LD
{

class VKDevice;

struct VKMemorySpec
{
    u32 MemoryType;
    u32 Size;
};

class VKMemory
{
public:
    VKMemory() = default;
    ~VKMemory() = default;

    VkDeviceMemory GetHandle() const
    {
        return mHandle;
    }

    VKMemorySpec GetSpec() const
    {
        return mSpec;
    }

    void Allocate(VKDevice& device, const VKMemorySpec& spec);
    void Free(VKDevice& device);

    void* Map(VKDevice& device);
    void Unmap(VKDevice& device);

private:
    VKMemorySpec mSpec;
    VkDeviceMemory mHandle = VK_NULL_HANDLE;
};

} // namespace LD