#include "Core/DSA/Include/Array.h"
#include "Core/RenderFX/Include/Pipelines/SwapChainTransferPipeline.h"
#include "Core/RenderFX/Include/Groups/FrameStaticGroup.h"
#include "Core/RenderFX/Include/Groups/ViewportGroup.h"

namespace LD
{

namespace Embed
{

extern void GetSwapChainTransferGLVS(unsigned int* size, const char** data);
extern void GetSwapChainTransferGLFS(unsigned int* size, const char** data);
extern void GetSwapChainTransferVKVS(unsigned int* size, const char** data);
extern void GetSwapChainTransferVKFS(unsigned int* size, const char** data);

} // namespace Embed

void SwapChainTransferPipeline::Startup(const SwapChainTransferPipelineInfo& info)
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
        Embed::GetSwapChainTransferVKVS(&vsSize, &vsData);
        Embed::GetSwapChainTransferVKFS(&fsSize, &fsData);
    }
    else
    {
        Embed::GetSwapChainTransferGLVS(&vsSize, &vsData);
        Embed::GetSwapChainTransferGLFS(&fsSize, &fsData);
    }

    RShaderInfo vertexSI;
    vertexSI.SourceType = RShaderSourceType::SPIRV;
    vertexSI.Type = RShaderType::VertexShader;
    vertexSI.Data = vsData;
    vertexSI.Size = vsSize;
    mDevice.CreateShader(mSwapChainTransferVS, vertexSI);

    RShaderInfo fragmentSI;
    fragmentSI.SourceType = RShaderSourceType::SPIRV;
    fragmentSI.Type = RShaderType::FragmentShader;
    fragmentSI.Data = fsData;
    fragmentSI.Size = fsSize;
    mDevice.CreateShader(mSwapChainTransferFS, fragmentSI);

    RPipelineInfo pipelineI{};
    pipelineI.Name = "SwapChainTransferPipeline";
    pipelineI.PrimitiveTopology = RPrimitiveTopology::TriangleList;
    pipelineI.VertexLayout.Slots = { 1, &slot };
    pipelineI.VertexShader = mSwapChainTransferVS;
    pipelineI.FragmentShader = mSwapChainTransferFS;
    pipelineI.PipelineLayout = info.PipelineLayout;
    pipelineI.RenderPass = info.RenderPass;
    pipelineI.DepthStencilState.DepthTestEnabled = false;
    mDevice.CreatePipeline(mHandle, pipelineI);
}

void SwapChainTransferPipeline::Cleanup()
{
    mDevice.DeleteShader(mSwapChainTransferFS);
    mDevice.DeleteShader(mSwapChainTransferVS);
    mDevice.DeletePipeline(mHandle);
    mDevice.ResetHandle();
}

RPipelineLayoutData SwapChainTransferPipeline::GetLayoutData() const
{
    RBindingGroupLayoutData group0 = FrameStaticGroup{}.GetLayoutData();
    RBindingGroupLayoutData group1 = ViewportGroup{}.GetLayoutData();
    RPipelineLayoutData data{};
    data.GroupLayouts = { group0, group1 };

    return data;
}

} // namespace LD