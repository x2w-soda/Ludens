#include "Core/RenderFX/Include/Pipelines/DeferredSSAOPipeline.h"
#include "Core/RenderFX/Include/Groups/FrameStaticGroup.h"
#include "Core/RenderFX/Include/Groups/ViewportGroup.h"
#include "Core/RenderFX/Include/Groups/SSAOGroup.h"
#include "Core/DSA/Include/Array.h"

namespace LD
{

namespace Embed
{

extern void GetDeferredSSAOGLVS(unsigned int* size, const char** data);
extern void GetDeferredSSAOGLFS(unsigned int* size, const char** data);
extern void GetDeferredSSAOVKVS(unsigned int* size, const char** data);
extern void GetDeferredSSAOVKFS(unsigned int* size, const char** data);

} // namespace Embed

DeferredSSAOPipeline::DeferredSSAOPipeline()
{
}

DeferredSSAOPipeline::~DeferredSSAOPipeline()
{
    LD_DEBUG_ASSERT(!mDevice);
}

void DeferredSSAOPipeline::Startup(const DeferredSSAOPipelineInfo& info)
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
        Embed::GetDeferredSSAOVKVS(&vsSize, &vsData);
        Embed::GetDeferredSSAOVKFS(&fsSize, &fsData);
    }
    else
    {
        Embed::GetDeferredSSAOGLVS(&vsSize, &vsData);
        Embed::GetDeferredSSAOGLFS(&fsSize, &fsData);
    }

    RShaderInfo vertexSI;
    vertexSI.SourceType = RShaderSourceType::SPIRV;
    vertexSI.Type = RShaderType::VertexShader;
    vertexSI.Data = vsData;
    vertexSI.Size = vsSize;
    mDevice.CreateShader(mDeferredSSAOVS, vertexSI);

    RShaderInfo fragmentSI;
    fragmentSI.SourceType = RShaderSourceType::SPIRV;
    fragmentSI.Type = RShaderType::FragmentShader;
    fragmentSI.Data = fsData;
    fragmentSI.Size = fsSize;
    mDevice.CreateShader(mDeferredSSAOFS, fragmentSI);

    RPipelineInfo pipelineI{};
    pipelineI.Name = "DeferredSSAOPipeline";
    pipelineI.PrimitiveTopology = RPrimitiveTopology::TriangleList;
    pipelineI.VertexLayout.Slots = { 1, &slot };
    pipelineI.VertexShader = mDeferredSSAOVS;
    pipelineI.FragmentShader = mDeferredSSAOFS;
    pipelineI.PipelineLayout = info.PipelineLayout;
    pipelineI.RenderPass = info.RenderPass;
    pipelineI.DepthStencilState.DepthTestEnabled = false;
    mDevice.CreatePipeline(mHandle, pipelineI);
}

void DeferredSSAOPipeline::Cleanup()
{
    mDevice.DeletePipeline(mHandle);
    mDevice.DeleteShader(mDeferredSSAOFS);
    mDevice.DeleteShader(mDeferredSSAOVS);
    mDevice.ResetHandle();
}

RPipelineLayoutData DeferredSSAOPipeline::GetLayoutData() const
{
    RBindingGroupLayoutData group0 = ViewportGroup{}.GetLayoutData();
    RBindingGroupLayoutData group1 = SSAOGroup{}.GetLayoutData();

    RPipelineLayoutData data{};
    data.GroupLayouts = { group0, group1 };

    return data;
}

} // namespace LD