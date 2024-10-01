#include "Core/RenderFX/Include/Pipelines/CubemapPipeline.h"
#include "Core/RenderFX/Include/Groups/FrameStaticGroup.h"
#include "Core/RenderFX/Include/Groups/ViewportGroup.h"
#include "Core/RenderFX/Include/Groups/CubemapGroup.h"
#include "Core/DSA/Include/Array.h"

namespace LD
{

namespace Embed
{

extern void GetCubemapGLVS(unsigned int* size, const char** data);
extern void GetCubemapGLFS(unsigned int* size, const char** data);
extern void GetCubemapVKVS(unsigned int* size, const char** data);
extern void GetCubemapVKFS(unsigned int* size, const char** data);

} // namespace Embed

void CubemapPipeline::Startup(const CubemapPipelineInfo& info)
{
    mDevice = info.Device;
    RBackend backend = mDevice.GetBackend();

    RVertexBufferSlot cubemapVertexSlot;
    Array<RVertexAttribute, 1> cubemapVertexAttr{
        { 0, RDataType::Vec3, false }
    };

    cubemapVertexSlot.Attributes = cubemapVertexAttr.GetView();
    cubemapVertexSlot.PollRate = RAttributePollRate::PerVertex;

    const char* vsData;
    unsigned int vsSize;
    const char* fsData;
    unsigned int fsSize;

    if (backend == RBackend::Vulkan)
    {
        Embed::GetCubemapVKVS(&vsSize, &vsData);
        Embed::GetCubemapVKFS(&fsSize, &fsData);
    }
    else
    {
        Embed::GetCubemapGLVS(&vsSize, &vsData);
        Embed::GetCubemapGLFS(&fsSize, &fsData);
    }

    RShaderInfo vertexSI;
    vertexSI.SourceType = RShaderSourceType::SPIRV;
    vertexSI.Type = RShaderType::VertexShader;
    vertexSI.Data = vsData;
    vertexSI.Size = vsSize;
    mDevice.CreateShader(mCubemapVS, vertexSI);

    RShaderInfo fragmentSI;
    fragmentSI.SourceType = RShaderSourceType::SPIRV;
    fragmentSI.Type = RShaderType::FragmentShader;
    fragmentSI.Data = fsData;
    fragmentSI.Size = fsSize;
    mDevice.CreateShader(mCubemapFS, fragmentSI);

    RPipelineInfo pipelineI{};
    pipelineI.Name = "CubemapPipeline";
    pipelineI.PrimitiveTopology = RPrimitiveTopology::TriangleList;
    pipelineI.VertexLayout.Slots = { 1, &cubemapVertexSlot };
    pipelineI.VertexShader = mCubemapVS;
    pipelineI.FragmentShader = mCubemapFS;
    pipelineI.PipelineLayout = info.CubemapPipelineLayout;
    pipelineI.RenderPass = info.RenderPass;
    pipelineI.DepthStencilState.DepthTestEnabled = true;
    pipelineI.DepthStencilState.DepthWriteEnabled = true;
    pipelineI.DepthStencilState.DepthCompareMode = RCompareMode::LessEqual;
    mDevice.CreatePipeline(mHandle, pipelineI);
}

void CubemapPipeline::Cleanup()
{
    mDevice.DeletePipeline(mHandle);
    mDevice.DeleteShader(mCubemapFS);
    mDevice.DeleteShader(mCubemapVS);
    mDevice.ResetHandle();
}

RPipelineLayoutData CubemapPipeline::GetLayoutData() const
{
    RBindingGroupLayoutData group0, group1, group2;
    group0 = FrameStaticGroup{}.GetLayoutData();
    group1 = ViewportGroup{}.GetLayoutData();
    group2 = CubemapGroup{}.GetLayoutData();

    RPipelineLayoutData data;
    data.GroupLayouts = { group0, group1, group2 };
    return data;
}

} // namespace LD