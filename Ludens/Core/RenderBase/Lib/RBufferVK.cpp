#include "Core/RenderBase/Include/VK/VKInfo.h"
#include "Core/RenderBase/Lib/RBufferVK.h"
#include "Core/RenderBase/Lib/RDeviceVK.h"

namespace LD
{

RBufferVK::RBufferVK()
{
}

RBufferVK::~RBufferVK()
{
    LD_DEBUG_ASSERT(ID == 0);
}

void RBufferVK::Startup(RBuffer& bufferH, const RBufferInfo& info, RDeviceVK& device)
{
    RBufferBase::Startup(bufferH, info, (RDeviceBase*)&device);
    VKContext& vkContext = device.Context;
    VKDevice& vkDevice = vkContext.GetDevice();

    VkBufferUsageFlags bufferUsage;
    switch (info.Type)
    {
    case RBufferType::VertexBuffer:
        bufferUsage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        break;
    case RBufferType::IndexBuffer:
        bufferUsage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        break;
    case RBufferType::UniformBuffer:
        bufferUsage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        break;
    default:
        LD_DEBUG_UNREACHABLE;
    }

    VkMemoryPropertyFlags memoryProperties;
    switch (info.MemoryUsage)
    {
    case RMemoryUsage::Immutable:
        memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        break;
    case RMemoryUsage::FrameDynamic:
        memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        break;
    default:
        LD_DEBUG_UNREACHABLE;
    }

    VKBufferInfo bufferI{};
    bufferI.CreateInfo = VKInfo::BufferCreate(info.Size, bufferUsage);
    ;
    bufferI.MemoryProperties = memoryProperties;

    if (bufferI.MemoryProperties & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    {
        // stage data to device local memory, data must be supplied
        LD_DEBUG_ASSERT(info.Size > 0 && info.Data);

        bufferI.CreateInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        Buffer.Startup(vkDevice, bufferI);
        Buffer.StageData(info.Size, info.Data, device.TransferCommandPool, vkDevice.GetTransferQueue());
        MemoryMap = nullptr;
    }
    else
    {
        Buffer.Startup(vkDevice, bufferI);
        MemoryMap = Buffer.Map();

        if (info.Size > 0 && info.Data)
        {
            memcpy(MemoryMap, info.Data, info.Size);
        }
    }
}

void RBufferVK::Cleanup(RBuffer& bufferH)
{
    RBufferBase::Cleanup(bufferH);

    if (MemoryMap)
    {
        Buffer.Unmap();
        MemoryMap = nullptr;
    }

    Buffer.Cleanup();
}

RResult RBufferVK::SetData(u32 offset, u32 size, const void* data)
{
    LD_DEBUG_ASSERT(MemoryMap != nullptr);

    memcpy((char*)MemoryMap + offset, data, size);

    return {};
}

} // namespace LD