#pragma once

#include "Core/RenderBase/Include/RShader.h"
#include "Core/RenderBase/Include/VK/VKShader.h"
#include "Core/RenderBase/Lib/RBase.h"

namespace LD
{

struct RDeviceVK;

struct RShaderVK : RShaderBase
{
    RShaderVK();
    RShaderVK(const RShaderVK&) = delete;
    RShaderVK(RShaderVK&&) = default;
    ~RShaderVK();

    RShaderVK& operator=(const RShaderVK&) = delete;
    RShaderVK& operator=(RShaderVK&&) noexcept = default;

    inline bool operator==(const RShaderVK& other) const
    {
        return ID == other.ID;
    }
    inline bool operator!=(const RShaderVK& other) const
    {
        return ID != other.ID;
    }

    void Startup(RShader& shaderH, const RShaderInfo& spec, RDeviceVK& device);
    void Cleanup(RShader& shaderH);

    VKShader ShaderModule;
};

} // namespace LD