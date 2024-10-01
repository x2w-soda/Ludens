#include "Core/DSA/Include/Array.h"
#include "Core/RenderFX/Include/Pipelines/SSAOBlurPipeline.h"
#include "Core/RenderFX/Include/Pipelines/DeferredSSAOPipeline.h"

namespace LD
{

namespace Embed
{

extern void GetSSAOBlurGLVS(unsigned int* size, const char** data);
extern void GetSSAOBlurGLFS(unsigned int* size, const char** data);
extern void GetSSAOBlurVKVS(unsigned int* size, const char** data);
extern void GetSSAOBlurVKFS(unsigned int* size, const char** data);

} // namespace Embed

SSAOBlurPipeline::SSAOBlurPipeline()
{
}

SSAOBlurPipeline::~SSAOBlurPipeline()
{
    LD_DEBUG_ASSERT(!mDevice);
}

void SSAOBlurPipeline::Startup(const SSAOBlurPipelineInfo& info)
{
    mDevice = info.Device;
    RBackend backend = mDevice.GetBackend();

    RVertexBufferSlot slot;
    Array<RVertexAttribute, 2> attrs{
        { 0, RDataType::Vec2, false }, // position
        { 1, RDataType::Vec2, false }, // texture uv
    };
    slot.Attributes = { attrs.Size(), attrs.Data() };

    const char* vsData;
    unsigned int vsSize;
    const char* fsData;
    unsigned int fsSize;

    if (backend == RBackend::Vulkan)
    {
        Embed::GetSSAOBlurVKVS(&vsSize, &vsData);
        Embed::GetSSAOBlurVKFS(&fsSize, &fsData);
    }
    else
    {
        Embed::GetSSAOBlurGLVS(&vsSize, &vsData);
        Embed::GetSSAOBlurGLFS(&fsSize, &fsData);
    }

    RShaderInfo vertexSI;
    vertexSI.SourceType = RShaderSourceType::SPIRV;
    vertexSI.Type = RShaderType::VertexShader;
    vertexSI.Data = vsData;
    vertexSI.Size = vsSize;
    mDevice.CreateShader(mSSAOBlurVS, vertexSI);

    RShaderInfo fragmentSI;
    fragmentSI.SourceType = RShaderSourceType::SPIRV;
    fragmentSI.Type = RShaderType::FragmentShader;
    fragmentSI.Data = fsData;
    fragmentSI.Size = fsSize;
    mDevice.CreateShader(mSSAOBlurFS, fragmentSI);

    RPipelineInfo pipelineI{};
    pipelineI.Name = "SSAOBlurPipeline";
    pipelineI.PrimitiveTopology = RPrimitiveTopology::TriangleList;
    pipelineI.VertexLayout.Slots = { 1, &slot };
    pipelineI.VertexShader = mSSAOBlurVS;
    pipelineI.FragmentShader = mSSAOBlurFS;
    pipelineI.PipelineLayout = info.PipelineLayout;
    pipelineI.RenderPass = info.RenderPass;
    pipelineI.DepthStencilState.DepthTestEnabled = false;
    mDevice.CreatePipeline(mHandle, pipelineI);
}

void SSAOBlurPipeline::Cleanup()
{
    mDevice.DeletePipeline(mHandle);
    mDevice.DeleteShader(mSSAOBlurFS);
    mDevice.DeleteShader(mSSAOBlurVS);
    mDevice.ResetHandle();
}

RPipelineLayoutData SSAOBlurPipeline::GetLayoutData() const
{
    // same pipeline layout as the main SSAO pipeline
    return DeferredSSAOPipeline{}.GetLayoutData();
}

} // namespace LD