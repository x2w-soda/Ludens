#pragma once

#include "Core/RenderBase/Include/VK/VKFrameBuffer.h"
#include "Core/RenderBase/Lib/RFrameBufferVK.h"
#include "Core/RenderBase/Lib/RBase.h"

namespace LD
{

struct RDeviceVK;

struct RFrameBufferVK : RFrameBufferBase
{
    RFrameBufferVK();
    RFrameBufferVK(const RFrameBufferVK&) = delete;
    ~RFrameBufferVK();

    RFrameBufferVK& operator=(const RFrameBufferVK&) = delete;

    void Startup(RFrameBuffer& frameBufferH, const RFrameBufferInfo& info, RDeviceVK& device);
    void Cleanup(RFrameBuffer& frameBufferH);

    virtual RResult Invalidate(const RFrameBufferInfo& info) override;

    VKFrameBuffer FrameBuffer;
};

} // namespace LD