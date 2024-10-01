#pragma once

#include "Core/RenderBase/Include/VK/VKRenderPass.h"
#include "Core/RenderBase/Include/RPass.h"
#include "Core/RenderBase/Lib/RPassVK.h"
#include "Core/RenderBase/Lib/RBase.h"

namespace LD
{

struct RDeviceVK;

struct RPassVK : RPassBase
{
    RPassVK();
    RPassVK(const RPassVK&) = delete;
    ~RPassVK();

    RPassVK& operator=(const RPassVK&) = delete;

    void Startup(RPass& passH, const RPassInfo& info, RDeviceVK& device);
    void Cleanup(RPass& passH);

    VKRenderPass RenderPass;
};

} // namespace LD