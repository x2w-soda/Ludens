#include <unordered_map>
#include <utility>
#include "Core/Math/Include/Mat3.h"
#include "Core/DSA/Include/Array.h"
#include "Core/Application/Include/Application.h"
#include "Core/RenderBase/Include/RPipeline.h"
#include "Core/RenderBase/Include/RShader.h"
#include "Core/RenderFX/Include/RMesh.h"
#include "Core/RenderFX/Include/Groups/CubemapGroup.h"
#include "Core/RenderFX/Include/Groups/ViewportGroup.h"
#include "Core/RenderService/Lib/RenderPassResources.h"
#include "Core/RenderService/Lib/FrameBufferResources.h"
#include "Core/RenderService/Lib/BindingGroupResources.h"
#include "Core/RenderService/Lib/PipelineResources.h"
#include "Core/RenderService/Lib/TextureResources.h"
#include "Core/RenderService/Lib/RenderUI.h"
#include "Core/RenderService/Lib/RenderContext.h"
#include "Core/RenderService/Include/RenderService.h"

namespace LD
{

struct MeshResource
{
    RMesh Mesh;
    RBuffer InstanceTransforms;
};

struct CubemapResource
{
    RTexture Cubemap;
    CubemapGroup CubemapBG;
};

/// maintain a draw list for each BeginViewport/EndViewport scope
struct DrawList
{
    Vec3 ViewPos;
    Mat4 ViewMat;
    Mat4 ProjMat;
};

struct WorldDrawList : DrawList
{
    Vector<std::pair<RRID, Mat4>> Meshes;
    RRID Cubemap;
};

struct ScreenDrawList : DrawList
{
    UIContext* UI = nullptr;
};

static RDevice sDevice;
static RRID sDirectionalLight;
static FrameStaticLightingUBO sLightingUBO;
static std::unordered_map<RRID, MeshResource> sMeshes;
static std::unordered_map<RRID, CubemapResource> sCubemaps;
static Vector<WorldDrawList> sWorldDrawLists;
static Vector<ScreenDrawList> sScreenDrawLists;

static void RenderServiceCallback(const RResult& result)
{
    LD_DEBUG_ASSERT(result.Type == RResultType::Ok);
}

void RenderService::Startup(RBackend backend)
{
    int width, height;
    auto& app = Application::GetSingleton();
    app.GetWindowSize(&width, &height);

    RDeviceInfo deviceI{};
    deviceI.Backend = backend;
    deviceI.Callback = RenderServiceCallback;
    CreateRenderDevice(sDevice, deviceI);

    mCtx = new RenderContext();
    mCtx->Startup(sDevice, width, height);
}

void RenderService::Cleanup()
{
    mCtx->Cleanup();
    delete mCtx;

    DeleteRenderDevice(sDevice);
}

void RenderService::GetDefaultFont(Ref<FontTTF>& ttf, Ref<FontGlyphTable>& table)
{
    ttf = mCtx->DefaultFontTTF;
    table = mCtx->DefaultFontAtlas.GetGlyphTable();
}

void RenderService::SetDefaultRenderPipeline(RenderPipeline pipeline)
{
    mCtx->DefaultRenderPipeline = pipeline;
}

void RenderService::SetLDRResult(LDRResult result)
{
    mCtx->DefaultLDRResult = result;
}

void RenderService::BeginFrame()
{
    // adapt to application framebuffer size
    int width, height;

    auto& app = Application::GetSingleton();
    app.GetWindowPixelSize(&width, &height);

    if (width != mCtx->ViewportWidth || height != mCtx->ViewportHeight)
    {
        OnViewportResize(width, height);
    }

    sWorldDrawLists.Clear();
    sScreenDrawLists.Clear();

    // upload frame static data
    FrameStaticGroup& group = mCtx->BindingGroups.GetFrameStaticGroup();
    RBuffer ubo = group.GetLightingUBO();
    ubo.SetData(0, sizeof(sLightingUBO), &sLightingUBO);

    sDevice.BeginFrame();

    mCtx->HasBeginFrame = true;
}

void RenderService::EndFrame()
{
    mCtx->HasBeginFrame = false;

    // Render world space objects to HDR color buffer
    WorldRenderPasses();

    // Tone mapping to LDR and render screen space objects
    ScreenRenderPasses();

    // Copy results to SwapChain FrameBuffer with gamma correction.
    SwapChainRenderPasses();

    sDevice.EndFrame();
}

void RenderService::BeginWorldViewport(const Vec3& viewpos, const Mat4& view, const Mat4& projection, RRID cubemap)
{
    LD_DEBUG_ASSERT(mCtx->HasBeginFrame);
    LD_DEBUG_ASSERT(!mCtx->HasBeginViewport);

    sWorldDrawLists.PushBack({});
    WorldDrawList& list = sWorldDrawLists.Back();
    list.ViewPos = viewpos;
    list.ViewMat = view;
    list.ProjMat = projection;
    list.Cubemap = cubemap;

    mCtx->HasBeginViewport = true;
}

void RenderService::EndWorldViewport()
{
    LD_DEBUG_ASSERT(mCtx->HasBeginViewport);

    mCtx->HasBeginViewport = false;
}

void RenderService::BeginScreenViewport()
{
    LD_DEBUG_ASSERT(!mCtx->HasBeginViewport);

    sScreenDrawLists.PushBack({});
    ScreenDrawList& list = sScreenDrawLists.Back();
    list.ViewPos = Vec3::Zero;
    list.ViewMat = Mat4::Identity;
    list.ProjMat = Mat4::Orthographic(0.0f, (float)mCtx->ViewportWidth, (float)mCtx->ViewportHeight, 0.0f, -1.0f, 1.0f);

    mCtx->HasBeginViewport = true;
}

void RenderService::EndScreenViewport()
{
    mCtx->HasBeginViewport = false;
}

void RenderService::CreateCubemap(RRID& id, int resolution, const void* data)
{
    id = CUID<CubemapResource>::Get();
    LD_DEBUG_ASSERT(sCubemaps.find(id) == sCubemaps.end());

    CubemapResource& res = sCubemaps[id];

    RTextureInfo cubemapI;
    cubemapI.Type = RTextureType::TextureCube;
    cubemapI.Format = RTextureFormat::RGBA8;
    cubemapI.Width = resolution;
    cubemapI.Height = resolution;
    cubemapI.Size = resolution * resolution * 4 * 6;
    cubemapI.Data = data;
    sDevice.CreateTexture(res.Cubemap, cubemapI);

    res.CubemapBG.Startup(sDevice, mCtx->BindingGroups.GetCubemapBGL(), res.Cubemap);
}

void RenderService::DeleteCubemap(RRID id)
{
    auto& iter = sCubemaps.find(id);

    if (iter == sCubemaps.end())
        return;

    CubemapResource& res = sCubemaps[id];
    res.CubemapBG.Cleanup();
    sDevice.DeleteTexture(res.Cubemap);

    sCubemaps.erase(iter);
}

void RenderService::CreateMesh(RRID& id, Ref<Model> model)
{
    id = CUID<MeshResource>::Get();
    LD_DEBUG_ASSERT(sMeshes.find(id) == sMeshes.end());

    MeshResource& res = sMeshes[id];

    RMeshInfo meshI;
    meshI.Device = sDevice;
    meshI.MaterialBGL = mCtx->BindingGroups.GetMaterialBGL();
    meshI.Data = model;
    res.Mesh.Startup(meshI);

    // store model matrix and normal matrix of one instance
    RBufferInfo bufferI;
    bufferI.MemoryUsage = RMemoryUsage::FrameDynamic;
    bufferI.Type = RBufferType::VertexBuffer;
    bufferI.Size = sizeof(Vec4) * 6;
    bufferI.Data = nullptr;
    sDevice.CreateBuffer(res.InstanceTransforms, bufferI);
}

void RenderService::DeleteMesh(RRID id)
{
    auto& iter = sMeshes.find(id);

    if (iter == sMeshes.end())
        return;

    MeshResource& res = iter->second;
    sDevice.DeleteBuffer(res.InstanceTransforms);
    res.Mesh.Cleanup();

    sMeshes.erase(iter);
}

void RenderService::CreateDirectionalLight(RRID& id, const Vec3& direction, const Vec3& color)
{
    LD_DEBUG_ASSERT(sDirectionalLight == 0 && "currently only allows single directional light");

    sDirectionalLight = id = GUID::Get();
    sLightingUBO.DirectionalLight.Dir = { direction, 0.0f };
    sLightingUBO.DirectionalLight.Color = { color, 0.0f };
}

void RenderService::DeleteDirectionalLight(RRID id)
{
    sDirectionalLight = 0;
}

void RenderService::DrawMesh(RRID id, const Mat4& transform)
{
    LD_DEBUG_ASSERT(mCtx->HasBeginViewport);
    LD_DEBUG_ASSERT(sMeshes.find(id) != sMeshes.end());

    sWorldDrawLists.Back().Meshes.PushBack({ id, transform });
}

void RenderService::DrawScreenUI(UIContext* ui)
{
    LD_DEBUG_ASSERT(mCtx->HasBeginViewport);

    sScreenDrawLists.Back().UI = ui;
}

void RenderService::OnViewportResize(int width, int height)
{
    printf("RenderService::OnViewportResize(%d,%d)\n", width, height);

    mCtx->OnViewportResize(width, height);
}

void RenderService::WorldRenderPasses()
{
    // GBuffer Pass
    {
        Array<RClearValue, 4> clearValues;
        clearValues[0].Color = { 0.0f, 0.0f, 0.0f, 1.0f };
        clearValues[1].Color = { 0.0f, 0.0f, 0.0f, 1.0f };
        clearValues[2].Color = { 0.0f, 0.0f, 0.0f, 1.0f };
        clearValues[3].DepthStencil = { 1.0f, 0 };

        RPassBeginInfo passBI;
        passBI.RenderPass = (RPass)mCtx->Passes.GetGBufferPass();
        passBI.FrameBuffer = (RFrameBuffer)mCtx->DefaultGBuffer;
        passBI.ClearValues = clearValues.GetView();
        sDevice.BeginRenderPass(passBI);

        sDevice.SetPipeline((RPipeline)mCtx->Pipelines.GetGBufferPipeline());
        sDevice.SetBindingGroup(0, (RBindingGroup)mCtx->WorldViewportGroup);

        // TODO: one viewport group per draw list
        LD_DEBUG_ASSERT(sWorldDrawLists.Size() <= 1);

        for (WorldDrawList& list : sWorldDrawLists)
        {
            RBuffer& ubo = mCtx->WorldViewportGroup.GetUBO();
            ViewportUBO viewportData;
            viewportData.PointLightStart = 0;
            viewportData.PointLightCount = 0;
            viewportData.ViewMat = list.ViewMat;
            viewportData.ProjMat = list.ProjMat;
            viewportData.ViewProjMat = list.ProjMat * list.ViewMat;
            viewportData.Size = { (float)mCtx->ViewportWidth, (float)mCtx->ViewportHeight };
            viewportData.ViewPos = list.ViewPos;
            ubo.SetData(0, sizeof(viewportData), &viewportData);

            for (auto& mesh : list.Meshes)
            {
                RRID id = mesh.first;
                const Mat4& modelMat = mesh.second;
                const Mat3 normalMat = Mat3::Transpose(Mat3::Inverse(Mat3(list.ViewMat * modelMat)));
                MeshResource& res = sMeshes[id];

                Array<Vec4, 6> instanceData;
                // 4x4 model matrix top 3 rows
                instanceData[0] = { modelMat[0][0], modelMat[1][0], modelMat[2][0], modelMat[3][0] };
                instanceData[1] = { modelMat[0][1], modelMat[1][1], modelMat[2][1], modelMat[3][1] };
                instanceData[2] = { modelMat[0][2], modelMat[1][2], modelMat[2][2], modelMat[3][2] };
                // 3x3 normal matrix columns
                instanceData[3] = { normalMat[0], 0.0f };
                instanceData[4] = { normalMat[1], 0.0f };
                instanceData[5] = { normalMat[2], 0.0f };

                res.InstanceTransforms.SetData(0, instanceData.ByteSize(), instanceData.Data());

                res.Mesh.Draw(
                    [&](RMesh::Batch& batch)
                    {
                        sDevice.SetBindingGroup(1, (RBindingGroup)batch.Material);
                        sDevice.SetVertexBuffer(0, batch.Vertices);
                        sDevice.SetVertexBuffer(1, res.InstanceTransforms);
                        sDevice.SetIndexBuffer(batch.Indices, RIndexType::u32);

                        RDrawIndexedInfo info{};
                        info.IndexCount = batch.IndexCount;
                        info.InstanceStart = 0;
                        info.InstanceCount = 1;
                        sDevice.DrawIndexed(info);
                    });
            }

            // render skybox after meshes
            auto iter = sCubemaps.find(list.Cubemap);
            if (iter != sCubemaps.end())
            {
                CubemapResource& res = sCubemaps[list.Cubemap];
                sDevice.SetPipeline((RPipeline)mCtx->Pipelines.GetCubemapPipeline());
                sDevice.SetBindingGroup(0, (RBindingGroup)mCtx->BindingGroups.GetFrameStaticGroup());
                sDevice.SetBindingGroup(1, (RBindingGroup)mCtx->WorldViewportGroup);
                sDevice.SetBindingGroup(2, (RBindingGroup)res.CubemapBG);
                sDevice.SetVertexBuffer(0, mCtx->CubeVBO);

                RDrawVertexInfo info{};
                info.VertexStart = 0;
                info.VertexCount = 36;
                sDevice.DrawVertex(info);
            }
        }

        sDevice.EndRenderPass();
    }

    // SSAO pass
    {
        RPassBeginInfo passBI;
        passBI.RenderPass = (RPass)mCtx->Passes.GetSSAOPass();
        passBI.FrameBuffer = (RFrameBuffer)mCtx->DefaultSSAOBuffer;
        sDevice.BeginRenderPass(passBI);

        sDevice.SetPipeline((RPipeline)mCtx->Pipelines.GetDeferredSSAOPipeline());
        sDevice.SetBindingGroup(0, (RBindingGroup)mCtx->WorldViewportGroup);
        sDevice.SetBindingGroup(1, (RBindingGroup)mCtx->BindingGroups.GetSSAOGroup());
        sDevice.SetVertexBuffer(0, mCtx->QuadVBO);

        RDrawVertexInfo drawInfo{};
        drawInfo.VertexCount = 6;
        sDevice.DrawVertex(drawInfo);

        sDevice.EndRenderPass();
    }

    // SSAO blur pass
    {
        RPassBeginInfo passBI;
        passBI.RenderPass = (RPass)mCtx->Passes.GetSSAOPass();
        passBI.FrameBuffer = (RFrameBuffer)mCtx->DefaultSSAOBlurBuffer;
        sDevice.BeginRenderPass(passBI);

        sDevice.SetPipeline((RPipeline)mCtx->Pipelines.GetSSAOBlurPipeline());
        sDevice.SetBindingGroup(0, (RBindingGroup)mCtx->WorldViewportGroup);
        sDevice.SetBindingGroup(1, (RBindingGroup)mCtx->BindingGroups.GetSSAOGroup());
        sDevice.SetVertexBuffer(0, mCtx->QuadVBO);

        RDrawVertexInfo drawInfo{};
        drawInfo.VertexCount = 6;
        sDevice.DrawVertex(drawInfo);

        sDevice.EndRenderPass();
    }

    // deferred lighting pass, render to HDR framebuffer
    {
        RPassBeginInfo passBI;
        passBI.RenderPass = (RPass)mCtx->Passes.GetColorPassHDR();
        passBI.FrameBuffer = (RFrameBuffer)mCtx->ColorBufferHDR;
        sDevice.BeginRenderPass(passBI);

        if (mCtx->DefaultRenderPipeline == RenderPipeline::BRDF)
            sDevice.SetPipeline((RPipeline)mCtx->Pipelines.GetDeferredBRDFPipeline());
        else    
            sDevice.SetPipeline((RPipeline)mCtx->Pipelines.GetDeferredBlinnPhongPipeline());

        sDevice.SetBindingGroup(0, (RBindingGroup)mCtx->BindingGroups.GetFrameStaticGroup());
        sDevice.SetBindingGroup(1, (RBindingGroup)mCtx->WorldViewportGroup);
        sDevice.SetVertexBuffer(0, mCtx->QuadVBO);

        RDrawVertexInfo drawInfo{};
        drawInfo.VertexCount = 6;
        sDevice.DrawVertex(drawInfo);

        sDevice.EndRenderPass();
    }
}

void RenderService::ScreenRenderPasses()
{
    RPassBeginInfo passBI;
    passBI.RenderPass = (RPass)mCtx->Passes.GetColorPassLDR();
    passBI.FrameBuffer = (RFrameBuffer)mCtx->ColorBufferLDR;
    sDevice.BeginRenderPass(passBI);

    ToneMappingGroup& toneGroup = mCtx->BindingGroups.GetToneMappingGroup();
    RBuffer toneUBO = toneGroup.GetUBO();

    ToneMappingUBO toneUBOData;
    toneUBOData.LDRResult = (int)mCtx->DefaultLDRResult;
    toneUBO.SetData(0, sizeof(ToneMappingUBO), &toneUBOData);

    // tone mapping, the HDR texture should already be bound in the ScreenViewportGroup
    sDevice.SetPipeline((RPipeline)mCtx->Pipelines.GetToneMappingPipeline());
    sDevice.SetBindingGroup(0, (RBindingGroup)mCtx->BindingGroups.GetFrameStaticGroup());
    sDevice.SetBindingGroup(1, (RBindingGroup)mCtx->WorldViewportGroup);
    sDevice.SetBindingGroup(2, (RBindingGroup)mCtx->ScreenViewportGroup);
    sDevice.SetBindingGroup(3, (RBindingGroup)mCtx->BindingGroups.GetToneMappingGroup());
    sDevice.SetVertexBuffer(0, mCtx->QuadVBO);

    RDrawVertexInfo drawInfo{};
    drawInfo.VertexCount = 6;
    sDevice.DrawVertex(drawInfo);

    // TODO: one viewport group per draw list
    LD_DEBUG_ASSERT(sScreenDrawLists.Size() <= 1);

    // Render Screen Space Objects
    for (ScreenDrawList& list : sScreenDrawLists)
    {
        if (!list.UI)
            continue;

        RBuffer& ubo = mCtx->ScreenViewportGroup.GetUBO();
        ViewportUBO viewportData;
        viewportData.PointLightStart = 0;
        viewportData.PointLightCount = 0;
        viewportData.ViewMat = list.ViewMat;
        viewportData.ProjMat = list.ProjMat;
        viewportData.ViewProjMat = list.ProjMat * list.ViewMat;
        viewportData.Size = { (float)mCtx->ViewportWidth, (float)mCtx->ViewportHeight };
        viewportData.ViewPos = list.ViewPos;
        ubo.SetData(0, sizeof(viewportData), &viewportData);

        sDevice.SetPipeline((RPipeline)mCtx->Pipelines.GetRectPipeline());
        sDevice.SetBindingGroup(0, (RBindingGroup)mCtx->ScreenViewportGroup);
        sDevice.SetBindingGroup(1, (RBindingGroup)mCtx->DefaultRectGroup);
        mCtx->DefaultRectBatcher.Reset();

        RenderUI(mCtx, list.UI);

        mCtx->DefaultRectBatcher.Commit();
    }

    sDevice.EndRenderPass();
}

void RenderService::SwapChainRenderPasses()
{
    RFrameBuffer swapChainFB;
    RPass swapChainRP;
    sDevice.GetSwapChainRenderPass(swapChainRP);
    sDevice.GetSwapChainFrameBuffer(swapChainFB);

    RClearValue clearColor;
    clearColor.Color = { 0.0f, 0.0f, 0.0f, 1.0f };

    RPassBeginInfo passBI;
    passBI.RenderPass = swapChainRP;
    passBI.FrameBuffer = swapChainFB;
    passBI.ClearValues = { 1, &clearColor };
    sDevice.BeginRenderPass(passBI);

    sDevice.SetPipeline((RPipeline)mCtx->Pipelines.GetSwapChainTransferPipeline());
    sDevice.SetBindingGroup(0, (RBindingGroup)mCtx->BindingGroups.GetFrameStaticGroup());
    sDevice.SetBindingGroup(1, (RBindingGroup)mCtx->ScreenViewportGroup);
    sDevice.SetVertexBuffer(0, mCtx->QuadVBO);

    RDrawVertexInfo drawInfo{};
    drawInfo.VertexCount = 6;
    sDevice.DrawVertex(drawInfo);

    sDevice.EndRenderPass();
}

} // namespace LD