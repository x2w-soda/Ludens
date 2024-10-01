#include "Core/DSA/Include/Array.h"
#include "Core/RenderFX/Include/Pipelines/LinePipeline.h"
#include "Core/RenderFX/Include/Groups/FrameStaticGroup.h"
#include "Core/RenderFX/Include/Groups/ViewportGroup.h"

namespace LD
{

namespace Embed
{

extern void GetLineGLVS(unsigned int* size, const char** data);
extern void GetLineGLFS(unsigned int* size, const char** data);
extern void GetLineVKVS(unsigned int* size, const char** data);
extern void GetLineVKFS(unsigned int* size, const char** data);

} // namespace Embed

LinePipeline::~LinePipeline()
{
    LD_DEBUG_ASSERT(!mDevice);
}

void LinePipeline::Startup(const LinePipelineInfo& info)
{
    mDevice = info.Device;
    RBackend backend = mDevice.GetBackend();

    RVertexBufferSlot slot{};
    Array<RVertexAttribute, 1> attributes{
        RVertexAttribute{ 0, RDataType::Vec3, false },  // Position
    };
    slot.PollRate = RAttributePollRate::PerVertex;
    slot.Attributes = { attributes.Size(), attributes.Data() };

    const char* vsData;
    unsigned int vsSize;
    const char* fsData;
    unsigned int fsSize;

    if (backend == RBackend::Vulkan)
    {
        Embed::GetLineVKVS(&vsSize, &vsData);
        Embed::GetLineVKFS(&fsSize, &fsData);
    }
    else
    {
        Embed::GetLineGLVS(&vsSize, &vsData);
        Embed::GetLineGLFS(&fsSize, &fsData);
    }

    RShaderInfo vertexSI;
    vertexSI.SourceType = RShaderSourceType::SPIRV;
    vertexSI.Type = RShaderType::VertexShader;
    vertexSI.Data = vsData;
    vertexSI.Size = vsSize;
    mDevice.CreateShader(mLineVS, vertexSI);

    RShaderInfo fragmentSI;
    fragmentSI.SourceType = RShaderSourceType::SPIRV;
    fragmentSI.Type = RShaderType::FragmentShader;
    fragmentSI.Data = fsData;
    fragmentSI.Size = fsSize;
    mDevice.CreateShader(mLineFS, fragmentSI);

    RPipelineInfo pipelineI{};
    pipelineI.Name = "LinePipeline";
    pipelineI.VertexLayout.Slots = { 1, &slot };
    pipelineI.VertexShader = mLineVS;
    pipelineI.FragmentShader = mLineFS;
    pipelineI.PrimitiveTopology = RPrimitiveTopology::LineList;
    pipelineI.PipelineLayout = info.LinePipelineLayout;
    pipelineI.RenderPass = info.RenderPass;
    pipelineI.DepthStencilState.DepthTestEnabled = false; // TODO:
    pipelineI.BlendState.BlendEnabled = true;
    pipelineI.BlendState.ColorSrcFactor = RBlendFactor::SrcAlpha;
    pipelineI.BlendState.ColorDstFactor = RBlendFactor::OneMinusSrcAlpha;
    pipelineI.BlendState.ColorBlendMode = RBlendMode::Add;
    pipelineI.BlendState.AlphaSrcFactor = RBlendFactor::One;
    pipelineI.BlendState.AlphaDstFactor = RBlendFactor::Zero;
    pipelineI.BlendState.AlphaBlendMode = RBlendMode::Add;
    mDevice.CreatePipeline(mHandle, pipelineI);
}

void LinePipeline::Cleanup()
{
    mDevice.DeletePipeline(mHandle);
    mDevice.DeleteShader(mLineFS);
    mDevice.DeleteShader(mLineVS);
    mDevice.ResetHandle();
}

RPipelineLayoutData LinePipeline::GetLayoutData() const
{
    RBindingGroupLayoutData group0 = FrameStaticGroup{}.GetLayoutData();
    RBindingGroupLayoutData group1 = ViewportGroup{}.GetLayoutData();
    RPipelineLayoutData data{};
    data.GroupLayouts = { group0, group1 };

    return data;
}

} // namespace LD