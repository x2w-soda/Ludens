#include "Core/DSA/Include/Array.h"
#include "Core/RenderFX/Include/Pipelines/PostProcess/ToneMappingPipeline.h"
#include "Core/RenderFX/Include/Groups/FrameStaticGroup.h"
#include "Core/RenderFX/Include/Groups/ViewportGroup.h"
#include "Core/RenderFX/Include/Groups/ToneMappingGroup.h"

namespace LD
{

namespace Embed
{

extern void GetToneMappingGLVS(unsigned int* size, const char** data);
extern void GetToneMappingGLFS(unsigned int* size, const char** data);
extern void GetToneMappingVKVS(unsigned int* size, const char** data);
extern void GetToneMappingVKFS(unsigned int* size, const char** data);

} // namespace Embed

void ToneMappingPipeline::Startup(const ToneMappingPipelineInfo& info)
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
        Embed::GetToneMappingVKVS(&vsSize, &vsData);
        Embed::GetToneMappingVKFS(&fsSize, &fsData);
    }
    else
    {
        Embed::GetToneMappingGLVS(&vsSize, &vsData);
        Embed::GetToneMappingGLFS(&fsSize, &fsData);
    }

    RShaderInfo vertexSI;
    vertexSI.SourceType = RShaderSourceType::SPIRV;
    vertexSI.Type = RShaderType::VertexShader;
    vertexSI.Data = vsData;
    vertexSI.Size = vsSize;
    mDevice.CreateShader(mToneMappingVS, vertexSI);

    RShaderInfo fragmentSI;
    fragmentSI.SourceType = RShaderSourceType::SPIRV;
    fragmentSI.Type = RShaderType::FragmentShader;
    fragmentSI.Data = fsData;
    fragmentSI.Size = fsSize;
    mDevice.CreateShader(mToneMappingFS, fragmentSI);

    RPipelineInfo pipelineI{};
    pipelineI.Name = "ToneMappingPipeline";
    pipelineI.PrimitiveTopology = RPrimitiveTopology::TriangleList;
    pipelineI.VertexLayout.Slots = { 1, &slot };
    pipelineI.VertexShader = mToneMappingVS;
    pipelineI.FragmentShader = mToneMappingFS;
    pipelineI.PipelineLayout = info.PipelineLayout;
    pipelineI.RenderPass = info.RenderPass;
    pipelineI.DepthStencilState.DepthTestEnabled = false;
    mDevice.CreatePipeline(mHandle, pipelineI);
}

void ToneMappingPipeline::Cleanup()
{
    mDevice.DeleteShader(mToneMappingFS);
    mDevice.DeleteShader(mToneMappingVS);
    mDevice.DeletePipeline(mHandle);
    mDevice.ResetHandle();
}

RPipelineLayoutData ToneMappingPipeline::GetLayoutData() const
{
    RBindingGroupLayoutData fs = FrameStaticGroup{}.GetLayoutData();
    RBindingGroupLayoutData viewport = ViewportGroup{}.GetLayoutData();
    RBindingGroupLayoutData tone = ToneMappingGroup{}.GetLayoutData();

    // frame static, world viewport, screen viewport, tone mapping
    RPipelineLayoutData data{};
    data.GroupLayouts = { fs, viewport, viewport, tone };

    return data;
}

} // namespace LD