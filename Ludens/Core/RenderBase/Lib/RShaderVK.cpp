#include "Core/RenderBase/Lib/RShaderVK.h"
#include "Core/RenderBase/Lib/RDeviceVK.h"
#include "Core/RenderBase/Lib/RDeriveVK.h"

namespace LD
{

RShaderVK::RShaderVK()
{
}

RShaderVK::~RShaderVK()
{
    LD_DEBUG_ASSERT(ID == 0);
}

void RShaderVK::Startup(RShader& shaderH, const RShaderInfo& info, RDeviceVK& device)
{
    RShaderBase::Startup(shaderH, info, (RDeviceBase*)&device);

    LD_DEBUG_ASSERT(info.SourceType == RShaderSourceType::SPIRV);

    VkShaderStageFlagBits stage;

    if (info.Type == RShaderType::VertexShader)
        stage = VK_SHADER_STAGE_VERTEX_BIT;
    else if (info.Type == RShaderType::FragmentShader)
        stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    else
        LD_DEBUG_UNREACHABLE;

    ShaderModule.SetName(info.Name)
        .SetShaderStage(stage)
        .SetShaderSource(info.Size, (const u32*)info.Data)
        .Startup(device.Context);
}

void RShaderVK::Cleanup(RShader& shaderH)
{
    RShaderBase::Cleanup(shaderH);

    ShaderModule.Cleanup();
}

} // namespace LD