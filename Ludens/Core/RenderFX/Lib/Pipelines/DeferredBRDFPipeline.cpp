#include "Core/RenderFX/Include/Pipelines/DeferredBRDFPipeline.h"
#include "Core/RenderFX/Include/Groups/FrameStaticGroup.h"
#include "Core/RenderFX/Include/Groups/ViewportGroup.h"
#include "Core/DSA/Include/Array.h"

namespace LD
{

namespace Embed
{

extern void GetDeferredBRDFGLVS(unsigned int* size, const char** data);
extern void GetDeferredBRDFGLFS(unsigned int* size, const char** data);
extern void GetDeferredBRDFVKVS(unsigned int* size, const char** data);
extern void GetDeferredBRDFVKFS(unsigned int* size, const char** data);

} // namespace Embed

void DeferredBRDFPipeline::Startup(const DeferredBRDFPipelineInfo& info)
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
        Embed::GetDeferredBRDFVKVS(&vsSize, &vsData);
        Embed::GetDeferredBRDFVKFS(&fsSize, &fsData);
    }
    else
    {
        Embed::GetDeferredBRDFGLVS(&vsSize, &vsData);
        Embed::GetDeferredBRDFGLFS(&fsSize, &fsData);
    }

    RShaderInfo vertexSI;
    vertexSI.SourceType = RShaderSourceType::SPIRV;
    vertexSI.Type = RShaderType::VertexShader;
    vertexSI.Data = vsData;
    vertexSI.Size = vsSize;
    mDevice.CreateShader(mDeferredBRDFVS, vertexSI);

    RShaderInfo fragmentSI;
    fragmentSI.SourceType = RShaderSourceType::SPIRV;
    fragmentSI.Type = RShaderType::FragmentShader;
    fragmentSI.Data = fsData;
    fragmentSI.Size = fsSize;
    mDevice.CreateShader(mDeferredBRDFFS, fragmentSI);

    RPipelineInfo pipelineI{};
    pipelineI.Name = "DeferredBRDFPipeline";
    pipelineI.PrimitiveTopology = RPrimitiveTopology::TriangleList;
    pipelineI.VertexLayout.Slots = { 1, &slot };
    pipelineI.VertexShader = mDeferredBRDFVS;
    pipelineI.FragmentShader = mDeferredBRDFFS;
    pipelineI.PipelineLayout = info.PipelineLayout;
    pipelineI.RenderPass = info.RenderPass;
    pipelineI.DepthStencilState.DepthTestEnabled = false;
    mDevice.CreatePipeline(mHandle, pipelineI);
}

void DeferredBRDFPipeline::Cleanup()
{
    mDevice.DeleteShader(mDeferredBRDFFS);
    mDevice.DeleteShader(mDeferredBRDFVS);
    mDevice.DeletePipeline(mHandle);
    mDevice.ResetHandle();
}

RPipelineLayoutData DeferredBRDFPipeline::GetLayoutData() const
{
    RBindingGroupLayoutData group0 = FrameStaticGroup{}.GetLayoutData();
    RBindingGroupLayoutData group1 = ViewportGroup{}.GetLayoutData();

    RPipelineLayoutData data{};
    data.GroupLayouts = { group0, group1 };

    return data;
}

} // namespace LD