#pragma once

#include "Core/RenderBase/Include/RPipeline.h"
#include "Core/RenderBase/Include/VK/VKPipeline.h"
#include "Core/RenderBase/Lib/RBase.h"

namespace LD
{

struct RDeviceVK;

struct RPipelineVK : RPipelineBase
{
    RPipelineVK();
    RPipelineVK(const RPipelineVK&) = delete;
    RPipelineVK(RPipelineVK&&) = default;
    ~RPipelineVK();

    RPipelineVK& operator=(const RPipelineVK&) = delete;
    RPipelineVK& operator=(RPipelineVK&&) noexcept = default;

    inline bool operator==(const RPipelineVK& other) const
    {
        return ID == other.ID;
    }

    inline bool operator!=(const RPipelineVK& other) const
    {
        return ID != other.ID;
    }

    void Startup(RPipeline& pipelineH, const RPipelineInfo& info, RDeviceVK& device);
    void Cleanup(RPipeline& pipelineH);

    VKPipeline Pipeline;
    VKPipelineLayout PipelineLayout;
};

} // namespace LD