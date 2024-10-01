#include "Core/Header/Include/Error.h"
#include "Core/RenderBase/Include/RShader.h"
#include "Core/RenderBase/Lib/RShaderGL.h"

namespace LD
{

RShaderGL::RShaderGL()
{
}

RShaderGL::~RShaderGL()
{
    LD_DEBUG_ASSERT(ID == 0);
}

void RShaderGL::Startup(RShader& shaderH, const RShaderInfo& info, RDeviceGL& device)
{
    RShaderBase::Startup(shaderH, info, (RDeviceBase*)&device);

    // copy shader source, deferred creation until RPipelineGL
    Source = { (const char*)info.Data, (size_t)info.Size };
}

void RShaderGL::Cleanup(RShader& shaderH)
{
    RShaderBase::Cleanup(shaderH);
}

} // namespace LD