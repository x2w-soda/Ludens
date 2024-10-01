#include "Core/RenderFX/Include/Pipelines/DeferredBlinnPhongPipeline.h"
#include "Core/RenderFX/Include/Groups/FrameStaticGroup.h"
#include "Core/RenderFX/Include/Groups/ViewportGroup.h"
#include "Core/DSA/Include/Array.h"

namespace LD {

namespace Embed {

extern void GetDeferredBlinnPhongGLVS(unsigned int* size, const char** data);
extern void GetDeferredBlinnPhongGLFS(unsigned int* size, const char** data);
extern void GetDeferredBlinnPhongVKVS(unsigned int* size, const char** data);
extern void GetDeferredBlinnPhongVKFS(unsigned int* size, const char** data);

} // namespace Embed

DeferredBlinnPhongPipeline::DeferredBlinnPhongPipeline()
{
}

DeferredBlinnPhongPipeline::~DeferredBlinnPhongPipeline()
{
}

void DeferredBlinnPhongPipeline::Startup(const DeferredBlinnPhongPipelineInfo& info)
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
        Embed::GetDeferredBlinnPhongVKVS(&vsSize, &vsData);
        Embed::GetDeferredBlinnPhongVKFS(&fsSize, &fsData);
    }
    else
    {
        Embed::GetDeferredBlinnPhongGLVS(&vsSize, &vsData);
        Embed::GetDeferredBlinnPhongGLFS(&fsSize, &fsData);
    }

    RShaderInfo vertexSI;
    vertexSI.SourceType = RShaderSourceType::SPIRV;
    vertexSI.Type = RShaderType::VertexShader;
    vertexSI.Data = vsData;
    vertexSI.Size = vsSize;
    mDevice.CreateShader(mDeferredBlinnPhongVS, vertexSI);

    RShaderInfo fragmentSI;
    fragmentSI.SourceType = RShaderSourceType::SPIRV;
    fragmentSI.Type = RShaderType::FragmentShader;
    fragmentSI.Data = fsData;
    fragmentSI.Size = fsSize;
    mDevice.CreateShader(mDeferredBlinnPhongFS, fragmentSI);

    RPipelineInfo pipelineI{};
    pipelineI.Name = "GBufferPipeline";
    pipelineI.PrimitiveTopology = RPrimitiveTopology::TriangleList;
    pipelineI.VertexLayout.Slots = { 1, &slot };
    pipelineI.VertexShader = mDeferredBlinnPhongVS;
    pipelineI.FragmentShader = mDeferredBlinnPhongFS;
    pipelineI.PipelineLayout = info.PipelineLayout;
    pipelineI.RenderPass = info.RenderPass;
    pipelineI.DepthStencilState.DepthTestEnabled = false;
    mDevice.CreatePipeline(mHandle, pipelineI);
}

void DeferredBlinnPhongPipeline::Cleanup()
{
    mDevice.DeleteShader(mDeferredBlinnPhongFS);
    mDevice.DeleteShader(mDeferredBlinnPhongVS);
    mDevice.DeletePipeline(mHandle);
    mDevice.ResetHandle();
}

RPipelineLayoutData DeferredBlinnPhongPipeline::GetLayoutData() const
{
    RBindingGroupLayoutData group0 = FrameStaticGroup{}.GetLayoutData();
    RBindingGroupLayoutData group1 = ViewportGroup{}.GetLayoutData();

    RPipelineLayoutData data{};
    data.GroupLayouts = { group0, group1 };

    return data;
}

} // namespace LD