#include "Core/RenderBase/Include/RPipeline.h"
#include "Core/RenderFX/Include/Pipelines/GBufferPipeline.h"
#include "Core/RenderFX/Include/Groups/ViewportGroup.h"
#include "Core/RenderFX/Include/Groups/MaterialGroup.h"
#include "Core/DSA/Include/Array.h"

namespace LD {

namespace Embed {

extern void GetGBufferGLVS(unsigned int* size, const char** data);
extern void GetGBufferGLFS(unsigned int* size, const char** data);
extern void GetGBufferVKVS(unsigned int* size, const char** data);
extern void GetGBufferVKFS(unsigned int* size, const char** data);

} // namespace Embed


GBufferPipeline::GBufferPipeline()
{
}

GBufferPipeline::~GBufferPipeline()
{
}

void GBufferPipeline::Startup(const GBufferPipelineInfo& info)
{
    mDevice = info.Device;
    RBackend backend = mDevice.GetBackend();

    Array<RVertexBufferSlot, 2> gbufferVertexSlots;
    Array<RVertexAttribute, 4> gbufferVertexAttr{
        { 0, RDataType::Vec3, false }, // position
        { 1, RDataType::Vec3, false }, // normals
        { 2, RDataType::Vec3, false }, // tangents
        { 3, RDataType::Vec2, false }, // UVs
    };
    gbufferVertexSlots[0].PollRate = RAttributePollRate::PerVertex;
    gbufferVertexSlots[0].Attributes = gbufferVertexAttr.GetView();

    // per-instance model matrix and normal matrix
    Array<RVertexAttribute, 6> gbufferInstanceAttr{
        { 4, RDataType::Vec4, false }, // 4x4 model matrix row 1
        { 5, RDataType::Vec4, false }, // 4x4 model matrix row 2
        { 6, RDataType::Vec4, false }, // 4x4 model matrix row 3
        { 7, RDataType::Vec4, false }, // 3x3 normal matrix column 1, w component reserved
        { 8, RDataType::Vec4, false }, // 3x3 normal matrix column 2, w component reserved
        { 9, RDataType::Vec4, false }, // 3x3 normal matrix column 3, w component reserved
    };
    gbufferVertexSlots[1].PollRate = RAttributePollRate::PerInstance;
    gbufferVertexSlots[1].Attributes = gbufferInstanceAttr.GetView();

    const char* vsData;
    unsigned int vsSize;
    const char* fsData;
    unsigned int fsSize;

    if (backend == RBackend::Vulkan)
    {
        Embed::GetGBufferVKVS(&vsSize, &vsData);
        Embed::GetGBufferVKFS(&fsSize, &fsData);
    }
    else
    {
        Embed::GetGBufferGLVS(&vsSize, &vsData);
        Embed::GetGBufferGLFS(&fsSize, &fsData);
    }

    RShaderInfo vertexSI;
    vertexSI.SourceType = RShaderSourceType::SPIRV;
    vertexSI.Type = RShaderType::VertexShader;
    vertexSI.Data = vsData;
    vertexSI.Size = vsSize;
    mDevice.CreateShader(mGBufferVS, vertexSI);

    RShaderInfo fragmentSI;
    fragmentSI.SourceType = RShaderSourceType::SPIRV;
    fragmentSI.Type = RShaderType::FragmentShader;
    fragmentSI.Data = fsData;
    fragmentSI.Size = fsSize;
    mDevice.CreateShader(mGBufferFS, fragmentSI);

    RPipelineInfo pipelineI{};
    pipelineI.Name = "GBufferPipeline";
    pipelineI.PrimitiveTopology = RPrimitiveTopology::TriangleList;
    pipelineI.VertexLayout.Slots = gbufferVertexSlots.GetView();
    pipelineI.VertexShader = mGBufferVS;
    pipelineI.FragmentShader = mGBufferFS;
    pipelineI.PipelineLayout = info.GBufferPipelineLayout;
    pipelineI.RenderPass = info.RenderPass;
    pipelineI.DepthStencilState.DepthTestEnabled = true;
    mDevice.CreatePipeline(mHandle, pipelineI);
}

void GBufferPipeline::Cleanup()
{
    mDevice.DeleteShader(mGBufferFS);
    mDevice.DeleteShader(mGBufferVS);
    mDevice.DeletePipeline(mHandle);
    mDevice.ResetHandle();
}

RPipelineLayoutData GBufferPipeline::GetLayoutData() const
{
    RBindingGroupLayoutData group0 = ViewportGroup{}.GetLayoutData();
    RBindingGroupLayoutData group1 = MaterialGroup{}.GetLayoutData();
    RPipelineLayoutData data{};
    data.GroupLayouts = { group0, group1 };

    return data;
}

} // namespace LD