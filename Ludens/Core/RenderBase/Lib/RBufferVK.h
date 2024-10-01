#pragma once

#include "Core/RenderBase/Include/VK/VKBuffer.h"
#include "Core/RenderBase/Lib/RBase.h"

namespace LD
{

struct RDeviceVK;

struct RBufferVK : RBufferBase
{
    RBufferVK();
    RBufferVK(const RBufferVK&) = delete;
    ~RBufferVK();

    RBufferVK& operator=(const RBufferVK&) = delete;

    void Startup(RBuffer& handle, const RBufferInfo& info, RDeviceVK& device);
    void Cleanup(RBuffer& handle);

    virtual RResult SetData(u32 offset, u32 size, const void* data) override;

    VKBuffer Buffer;
    void* MemoryMap;
};

} // namespace LD