#pragma once

#include <string>
#include "Core/RenderBase/Include/RShader.h"
#include "Core/RenderBase/Lib/RBase.h"
#include "Core/RenderBase/Include/GL/GLProgram.h"

namespace LD
{

struct RDeviceGL;

struct RShaderGL : RShaderBase
{
    RShaderGL();
    RShaderGL(const RShaderGL&) = delete;
    RShaderGL(RShaderGL&&) = default;
    ~RShaderGL();

    RShaderGL& operator=(const RShaderGL&) = delete;
    RShaderGL& operator=(RShaderGL&&) noexcept = default;

    inline bool operator==(const RShaderGL& other) const
    {
        return ID == other.ID;
    }
    inline bool operator!=(const RShaderGL& other) const
    {
        return ID != other.ID;
    }

    void Startup(RShader& shaderH, const RShaderInfo& spec, RDeviceGL& device);
    void Cleanup(RShader& shaderH);

    std::string Source;
};

} // namespace LD