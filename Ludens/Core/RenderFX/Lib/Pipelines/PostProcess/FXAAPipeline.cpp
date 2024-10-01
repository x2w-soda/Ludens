#include "Core/RenderFX/Include/Pipelines/PostProcess/FXAAPipeline.h"
#include "Core/RenderFX/Include/Groups/FrameStaticGroup.h"
#include "Core/RenderFX/Include/Groups/ViewportGroup.h"
#include "Core/DSA/Include/Array.h"

namespace LD
{

namespace Embed
{

extern void GetFXAAGLVS(unsigned int* size, const char** data);
extern void GetFXAAGLFS(unsigned int* size, const char** data);
extern void GetFXAAVKVS(unsigned int* size, const char** data);
extern void GetFXAAVKFS(unsigned int* size, const char** data);

} // namespace Embed

void FXAAPipeline::Startup(const FXAAPipelineInfo& info)
{
    mDevice = info.Device;
    RBackend backend = mDevice.GetBackend();

    RVertexBufferSlot slot{};
    Array<RVertexAttribute, 4> attributes{
        RVertexAttribute{ 0, RDataType::Vec2, false }, // Pos
        RVertexAttribute{ 1, RDataType::Vec2, false }, // TexUV
    };
    slot.PollRate = RAttributePollRate::PerVertex;
    slot.Attributes = { attributes.Size(), attributes.Data() };

    const char* vsData;
    unsigned int vsSize;
    const char* fsData;
    unsigned int fsSize;

    if (backend == RBackend::Vulkan)
    {
        Embed::GetFXAAVKVS(&vsSize, &vsData);
        Embed::GetFXAAVKFS(&fsSize, &fsData);
    }
    else
    {
        Embed::GetFXAAGLVS(&vsSize, &vsData);
        Embed::GetFXAAGLFS(&fsSize, &fsData);
    }

    RShaderInfo vertexSI;
    vertexSI.SourceType = RShaderSourceType::SPIRV;
    vertexSI.Type = RShaderType::VertexShader;
    vertexSI.Data = vsData;
    vertexSI.Size = vsSize;
    mDevice.CreateShader(mFXAAVS, vertexSI);

    RShaderInfo fragmentSI;
    fragmentSI.SourceType = RShaderSourceType::SPIRV;
    fragmentSI.Type = RShaderType::FragmentShader;
    fragmentSI.Data = fsData;
    fragmentSI.Size = fsSize;
    mDevice.CreateShader(mFXAAFS, fragmentSI);

    RPipelineInfo pipelineI{};
    pipelineI.Name = "FXAAPipeline";
    pipelineI.VertexLayout.Slots = { 1, &slot };
    pipelineI.VertexShader = mFXAAVS;
    pipelineI.FragmentShader = mFXAAFS;
    pipelineI.PrimitiveTopology = RPrimitiveTopology::TriangleList;
    pipelineI.PipelineLayout = info.FXAAPipelineLayout;
    pipelineI.RenderPass = info.RenderPass;
    pipelineI.DepthStencilState.DepthTestEnabled = false;
    pipelineI.BlendState.BlendEnabled = false;
    mDevice.CreatePipeline(mHandle, pipelineI);
}

void FXAAPipeline::Cleanup()
{
    mDevice.DeleteShader(mFXAAFS);
    mDevice.DeleteShader(mFXAAVS);
    mDevice.DeletePipeline(mHandle);
    mDevice.ResetHandle();
}

RPipelineLayoutData FXAAPipeline::GetLayoutData() const
{
    RBindingGroupLayoutData fs = FrameStaticGroup{}.GetLayoutData();
    RBindingGroupLayoutData viewport = ViewportGroup{}.GetLayoutData();

    RPipelineLayoutData data{};
    data.GroupLayouts = { fs, viewport };

    return data;
}

} // namespace LD