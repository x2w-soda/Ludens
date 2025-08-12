#include <Ludens/Header/Assert.h>
#include <Ludens/Log/Log.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/RenderBackend/RUtil.h>
#include <Ludens/RenderComponent/DualKawaseComponent.h>
#include <Ludens/RenderComponent/ForwardRenderComponent.h>
#include <Ludens/RenderComponent/Layout/PipelineLayouts.h>
#include <Ludens/RenderComponent/Layout/SetLayouts.h>
#include <Ludens/RenderComponent/Pipeline/RMeshPipeline.h>
#include <Ludens/RenderComponent/SceneOverlayComponent.h>
#include <Ludens/RenderComponent/ScreenPickComponent.h>
#include <Ludens/RenderComponent/ScreenRenderComponent.h>
#include <Ludens/RenderGraph/RGraph.h>
#include <Ludens/RenderServer/RServer.h>
#include <Ludens/System/Memory.h>
#include <vector>

namespace LD {

static Log sLog("RServer");

/// @brief Render server implementation.
class RServerObj
{
    friend class RServer;

public:
    RServerObj(const RServerInfo& serviceI);
    ~RServerObj();

    void next_frame(const RServerFrameInfo& frameI);
    void submit_frame();

    void scene_pass(const RServerScenePass& sceneP);
    void editor_pass(const RServerEditorPass& editorP);
    void editor_overlay_pass(const RServerEditorOverlayPass& editorOP);

private:
    struct Frame
    {
        RBuffer ubo;
        RSet frameSet;
    };

    static void forward_rendering(ForwardRenderComponent renderer, void* user);

    bool pickid_is_gizmo(uint32_t pickID);
    RUID pickid_to_ruid(uint32_t pickID);
    uint32_t ruid_to_pickid(RUID ruid);

private:
    RDevice mDevice;
    RGraph mGraph;
    RSetPool mFrameSetPool;
    RImage mFontAtlasImage;
    RImage mCubemapImage;
    Camera mMainCamera;
    RMeshBlinnPhongPipeline mMeshPipeline;
    RUID mRUIDCtr;
    RServerTransformCallback mTransformCallback;
    Vec2 mSceneExtent;
    Vec2 mScreenExtent;
    std::vector<Frame> mFrames;
    std::vector<RCommandPool> mCmdPools;
    std::vector<RCommandList> mCmdLists;
    std::unordered_map<RUID, RMesh*> mMeshes; // TODO: this is terrible for cache locality
    RFormat mDepthStencilFormat;              /// default depth stencil format
    RFormat mColorFormat;                     /// default color format
    RSampleCountBit mMSAA;                    /// number of samples during MSAA, if enabled
    RUID mSceneOutlineSubject;                /// subject to be outlined in scene render pass
    uint32_t mFramesInFlight;                 /// number of frames in flight
    uint32_t mFrameIndex;                     /// [0, mFramesInFlight)
    FontAtlas mFontAtlas;                     /// default font atlas for text rendering
    const char* mLastComponent;               /// last render component
    const char* mLastColorAttachment;         /// last scene color attachment output
    const char* mLastIDFlagsAttachment;       /// last scene ID flags attachment output
    void* mTransformCallbackUser;
    bool mHasRenderedScene;
};

RServerObj::RServerObj(const RServerInfo& serverI)
    : mDevice(serverI.device), mColorFormat(RFORMAT_RGBA8), mFontAtlas(serverI.fontAtlas), mRUIDCtr(1)
{
    RSampleCountBit supportedMSCount = mDevice.get_max_sample_count();
    mMSAA = supportedMSCount >= RSAMPLE_COUNT_4_BIT ? RSAMPLE_COUNT_4_BIT : supportedMSCount;
    sLog.info("msaa {} bits suported, using {} sample bits", (int)supportedMSCount, (int)mMSAA);

    uint32_t count;
    RFormat depthStencilFormats[8];
    mDevice.get_depth_stencil_formats(depthStencilFormats, count);
    mDepthStencilFormat = depthStencilFormats[0];
    mTransformCallback = nullptr;
    mTransformCallbackUser = nullptr;

    //
    // Render Server Resources
    //

    Bitmap atlasBitmap = mFontAtlas.get_bitmap();
    RImageInfo imageI = RUtil::make_2d_image_info(RIMAGE_USAGE_SAMPLED_BIT | RIMAGE_USAGE_TRANSFER_DST_BIT, RFORMAT_R8, atlasBitmap.width(), atlasBitmap.height());
    imageI.sampler = {RFILTER_LINEAR, RFILTER_LINEAR, RSAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE};
    mFontAtlasImage = mDevice.create_image(imageI);
    RStager stager(mDevice, RQUEUE_TYPE_GRAPHICS);
    stager.add_image_data(mFontAtlasImage, atlasBitmap.data(), RIMAGE_LAYOUT_SHADER_READ_ONLY);

    RSamplerInfo cubemapSamplerI;
    cubemapSamplerI.filter = RFILTER_LINEAR;
    cubemapSamplerI.mipmapFilter = RFILTER_LINEAR;
    cubemapSamplerI.addressMode = RSAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

    Bitmap cubemapFaces = serverI.cubemapFaces;
    imageI = RUtil::make_cube_image_info(RIMAGE_USAGE_SAMPLED_BIT | RIMAGE_USAGE_TRANSFER_DST_BIT, RFORMAT_RGBA8, cubemapFaces.width(), cubemapSamplerI);
    mCubemapImage = mDevice.create_image(imageI);
    stager.add_image_data(mCubemapImage, cubemapFaces.data(), RIMAGE_LAYOUT_SHADER_READ_ONLY);

    stager.submit(mDevice.get_graphics_queue());

    mMeshPipeline = RMeshBlinnPhongPipeline::create(mDevice);

    //
    // Frames In Flight Resources
    //

    mFramesInFlight = mDevice.get_frames_in_flight_count();

    RSetPoolInfo setPoolI;
    setPoolI.layout = sFrameSetLayout;
    setPoolI.maxSets = mFramesInFlight;
    mFrameSetPool = mDevice.create_set_pool(setPoolI);

    mFrames.resize(mFramesInFlight);
    mCmdPools.resize(mFramesInFlight);
    mCmdLists.resize(mFramesInFlight);

    for (uint32_t i = 0; i < mFramesInFlight; i++)
    {
        mCmdPools[i] = mDevice.create_command_pool({RQUEUE_TYPE_GRAPHICS});
        mCmdLists[i] = mCmdPools[i].allocate();

        Frame& frame = mFrames[i];
        frame.ubo = mDevice.create_buffer({.usage = RBUFFER_USAGE_UNIFORM_BIT, .size = sizeof(FrameUBO), .hostVisible = true});
        frame.ubo.map();
        frame.frameSet = mFrameSetPool.allocate();
        RSetBufferUpdateInfo bufferUpdateI = RUtil::make_single_set_buffer_udpate_info(frame.frameSet, 0, RBINDING_TYPE_UNIFORM_BUFFER, &frame.ubo);
        mDevice.update_set_buffers(1, &bufferUpdateI);

        RImageLayout layout = RIMAGE_LAYOUT_SHADER_READ_ONLY;
        RSetImageUpdateInfo imageUpdateI = RUtil::make_single_set_image_update_info(frame.frameSet, 1, RBINDING_TYPE_COMBINED_IMAGE_SAMPLER, &layout, &mCubemapImage);
        mDevice.update_set_images(1, &imageUpdateI);
    }
}

RServerObj::~RServerObj()
{
    mDevice.wait_idle();

    RGraph::release(mDevice);

    for (auto ite : mMeshes)
    {
        RMesh* mesh = ite.second;
        mesh->destroy();
        heap_delete<RMesh>(mesh);
    }
    mMeshes.clear();

    for (uint32_t i = 0; i < mFramesInFlight; i++)
    {
        Frame& frame = mFrames[i];
        frame.ubo.unmap();
        mDevice.destroy_buffer(frame.ubo);
        mDevice.destroy_command_pool(mCmdPools[i]);
    }

    mDevice.destroy_set_pool(mFrameSetPool);

    RMeshBlinnPhongPipeline::destroy(mMeshPipeline);

    mDevice.destroy_image(mFontAtlasImage);
    mDevice.destroy_image(mCubemapImage);
}

void RServerObj::next_frame(const RServerFrameInfo& frameI)
{
    RSemaphore imageAcquired, presentReady;
    RFence frameComplete;
    uint32_t swapIdx;
    swapIdx = mDevice.next_frame(imageAcquired, presentReady, frameComplete);

    mSceneExtent = frameI.sceneExtent;
    mScreenExtent = frameI.screenExtent;
    mFrameIndex = mDevice.get_frame_index();
    mCmdPools[mFrameIndex].reset();
    RCommandList list = mCmdLists[mFrameIndex];
    Frame& frame = mFrames[mFrameIndex];

    RGraphInfo graphI{};
    graphI.device = mDevice;
    graphI.list = list;
    graphI.presentReady = presentReady;
    graphI.imageAcquired = imageAcquired;
    graphI.frameComplete = frameComplete;
    graphI.swapchainImage = mDevice.get_swapchain_color_attachment(swapIdx);
    graphI.screenWidth = (uint32_t)mScreenExtent.x;
    graphI.screenHeight = (uint32_t)mScreenExtent.y;
    mGraph = RGraph::create(graphI);

    //
    // Update Frame Set
    //

    mMainCamera = frameI.mainCamera;

    FrameUBO uboData;
    uboData.projMat = mMainCamera.get_proj();
    uboData.viewMat = mMainCamera.get_view();
    uboData.viewProjMat = uboData.projMat * uboData.viewMat;
    uboData.viewPos = Vec4(mMainCamera.get_pos(), 0.0f);
    uboData.dirLight = Vec4(0.0f, 1.0f, 0.0f, 0.0f); // TODO: RUID DirectionalLight
    uboData.screenExtent = mScreenExtent;
    uboData.sceneExtent = mSceneExtent;
    uboData.envPhase = 0; // TODO: expose
    frame.ubo.map_write(0, sizeof(uboData), &uboData);

    mLastComponent = nullptr;
    mLastColorAttachment = nullptr;
    mLastIDFlagsAttachment = nullptr;
    mHasRenderedScene = false;
}

void RServerObj::submit_frame()
{
    // blit to swapchain image and submit
    mGraph.connect_swapchain_image(mLastComponent, mLastColorAttachment);
    mGraph.submit();
    RGraph::destroy(mGraph);

    mDevice.present_frame();
}

void RServerObj::scene_pass(const RServerScenePass& sceneP)
{
    Frame& frame = mFrames[mFrameIndex];
    RClearColorValue clearColor = RUtil::make_clear_color(0.1f, 0.1f, 0.1f, 1.0f);
    RClearDepthStencilValue clearDS = {.depth = 1.0f, .stencil = 0};

    mSceneOutlineSubject = sceneP.overlay.enabled ? sceneP.overlay.outlineRUID : 0;
    mTransformCallback = sceneP.transformCallback;
    mTransformCallbackUser = sceneP.user;

    ForwardRenderComponentInfo forwardI{};
    forwardI.width = (uint32_t)mSceneExtent.x;
    forwardI.height = (uint32_t)mSceneExtent.y;
    forwardI.colorFormat = mColorFormat;
    forwardI.clearColor = clearColor;
    forwardI.depthStencilFormat = mDepthStencilFormat;
    forwardI.clearDepthStencil = clearDS;
    forwardI.samples = mMSAA;
    ForwardRenderComponent sceneFR = ForwardRenderComponent::add(mGraph, forwardI, frame.frameSet, &RServerObj::forward_rendering, this);

    if (sceneP.overlay.enabled) // mesh outlining and gizmo rendering is provided by the SceneOverlayComponent
    {
        SceneOverlayComponentInfo overlayI{};
        overlayI.colorFormat = mColorFormat;
        overlayI.depthStencilFormat = mDepthStencilFormat;
        overlayI.width = mSceneExtent.x;
        overlayI.height = mSceneExtent.y;
        overlayI.gizmoMSAA = mMSAA;
        overlayI.gizmoType = sceneP.overlay.gizmoType;
        overlayI.gizmoCenter = sceneP.overlay.gizmoCenter;
        overlayI.gizmoScale = sceneP.overlay.gizmoScale;
        overlayI.gizmoColorX = sceneP.overlay.gizmoColor.axisX;
        overlayI.gizmoColorY = sceneP.overlay.gizmoColor.axisY;
        overlayI.gizmoColorZ = sceneP.overlay.gizmoColor.axisZ;
        overlayI.gizmoColorXY = sceneP.overlay.gizmoColor.planeXY;
        overlayI.gizmoColorXZ = sceneP.overlay.gizmoColor.planeXZ;
        overlayI.gizmoColorYZ = sceneP.overlay.gizmoColor.planeYZ;
        SceneOverlayComponent overlayC = SceneOverlayComponent::add(mGraph, overlayI);
        mGraph.connect_image(sceneFR.component_name(), sceneFR.out_color_name(), overlayC.component_name(), overlayC.in_color_name());
        mGraph.connect_image(sceneFR.component_name(), sceneFR.out_idflags_name(), overlayC.component_name(), overlayC.in_idflags_name());
        mLastComponent = overlayC.component_name();
        mLastColorAttachment = overlayC.out_color_name();
        mLastIDFlagsAttachment = overlayC.out_idflags_name();
    }
    else
    {
        mLastComponent = sceneFR.component_name();
        mLastColorAttachment = sceneFR.out_color_name();
        mLastIDFlagsAttachment = sceneFR.out_idflags_name();
    }

    mHasRenderedScene = true;
}

void RServerObj::editor_pass(const RServerEditorPass& editorP)
{
    LD_ASSERT(mHasRenderedScene && mLastComponent && mLastColorAttachment && mLastIDFlagsAttachment);

    ScreenRenderComponentInfo screenRCI{};
    screenRCI.format = mColorFormat;
    screenRCI.onDrawCallback = editorP.renderCallback;
    screenRCI.user = editorP.user;
    screenRCI.hasInputImage = false;
    screenRCI.hasSampledImage = true;
    screenRCI.clearColor = 0x101010FF;
    screenRCI.name = "editor";
    ScreenRenderComponent editorSRC = ScreenRenderComponent::add(mGraph, screenRCI);
    mGraph.connect_image(mLastComponent, mLastColorAttachment, editorSRC.component_name(), editorSRC.sampled_name());

    ScreenPickComponentInfo pickCI{};
    pickCI.pickQueryCount = 0;
    if (editorP.sceneMousePickQuery)
    {
        pickCI.pickQueryCount = 1;
        pickCI.pickPositions = editorP.sceneMousePickQuery;
    }
    ScreenPickComponent screenPick = ScreenPickComponent::add(mGraph, pickCI);
    mGraph.connect_image(mLastComponent, mLastIDFlagsAttachment, screenPick.component_name(), screenPick.input_name());

    mLastComponent = editorSRC.component_name();
    mLastColorAttachment = editorSRC.io_name();

    // NOTE: The results are actually from mFramesInFlight frames ago,
    //       stalling the GPU just to acquire results in the same frame
    //       would be terrible for CPU-GPU concurrency.
    //       See ScreenPickComponent implementation.
    std::vector<ScreenPickResult> pickResults;
    screenPick.get_results(pickResults);
    if (pickResults.empty())
    {
        if (editorP.scenePickCallback)
            editorP.scenePickCallback((SceneOverlayGizmoID)0, (RUID)0, editorP.user);
        return;
    }

    const ScreenPickResult& pickResult = pickResults.front();

    if (editorP.scenePickCallback && pickid_is_gizmo(pickResult.id))
    {
        editorP.scenePickCallback((SceneOverlayGizmoID)pickResult.id, 0, editorP.user);
    }
    else if (editorP.scenePickCallback)
    {
        RUID resultRUID = pickid_to_ruid(pickResult.id);
        editorP.scenePickCallback((SceneOverlayGizmoID)0, resultRUID, editorP.user);
    }
}

void RServerObj::editor_overlay_pass(const RServerEditorOverlayPass& editorOP)
{
    DualKawaseComponentInfo blurCI{};
    blurCI.format = mColorFormat;
    blurCI.mixColor = editorOP.blurMixColor;
    blurCI.mixFactor = editorOP.blurMixFactor;
    DualKawaseComponent blurC = DualKawaseComponent::add(mGraph, blurCI);
    mGraph.connect_image(mLastComponent, mLastColorAttachment, blurC.component_name(), blurC.input_name());

    ScreenRenderComponentInfo screenRCI{};
    screenRCI.format = mColorFormat;
    screenRCI.onDrawCallback = editorOP.renderCallback;
    screenRCI.user = editorOP.user;
    screenRCI.hasInputImage = true;
    screenRCI.hasSampledImage = true;
    screenRCI.name = "editor_overlay";
    ScreenRenderComponent editorSRC = ScreenRenderComponent::add(mGraph, screenRCI);
    mGraph.connect_image(mLastComponent, mLastColorAttachment, editorSRC.component_name(), editorSRC.io_name());
    mGraph.connect_image(blurC.component_name(), blurC.output_name(), editorSRC.component_name(), editorSRC.sampled_name());

    mLastComponent = editorSRC.component_name();
    mLastColorAttachment = editorSRC.io_name();
}

// NOTE: This is super early placeholder scene renderer implementation.
//       Once other engine subsystems such as Assets and Scenes are resolved,
//       we will come back and replace this silly procedure.
void RServerObj::forward_rendering(ForwardRenderComponent renderer, void* user)
{
    LD_PROFILE_SCOPE;

    RServerObj& self = *(RServerObj*)user;
    RPipeline meshPipeline = self.mMeshPipeline.handle();

    renderer.set_mesh_pipeline(meshPipeline);

    // render Color and 16-bit ID
    meshPipeline.set_color_write_mask(0, RCOLOR_COMPONENT_R_BIT | RCOLOR_COMPONENT_G_BIT | RCOLOR_COMPONENT_B_BIT | RCOLOR_COMPONENT_A_BIT);
    meshPipeline.set_color_write_mask(1, RCOLOR_COMPONENT_R_BIT | RCOLOR_COMPONENT_G_BIT);
    meshPipeline.set_depth_test_enable(true);

    RMeshBlinnPhongPipeline::PushConstant pc;

    // render static mesh
    for (auto ite : self.mMeshes)
    {
        pc.model = self.mTransformCallback(ite.first, self.mTransformCallbackUser);
        pc.id = self.ruid_to_pickid(ite.first);
        pc.flags = 0;

        renderer.set_push_constant(sRMeshPipelineLayout, 0, sizeof(pc), &pc);
        renderer.draw_mesh(*ite.second);
    }

    // render flag hints for object outlining
    RUID outlineSubject = self.mSceneOutlineSubject;
    if (outlineSubject != 0 && self.mMeshes.contains(outlineSubject))
    {
        // render to 16-bit flags only
        meshPipeline.set_color_write_mask(0, 0);
        meshPipeline.set_color_write_mask(1, RCOLOR_COMPONENT_B_BIT | RCOLOR_COMPONENT_A_BIT);
        meshPipeline.set_depth_test_enable(false);

        pc.model = self.mTransformCallback(outlineSubject, self.mTransformCallbackUser);
        pc.id = 0;    // not written to color attachment due to write masks
        pc.flags = 1; // currently any non-zero flag value indicates mesh that requires outlining

        renderer.set_push_constant(sRMeshPipelineLayout, 0, sizeof(pc), &pc);
        renderer.draw_mesh(*self.mMeshes[outlineSubject]);
    }

    renderer.draw_skybox();
}

bool RServerObj::pickid_is_gizmo(uint32_t pickID)
{
    return 1 <= pickID && pickID <= SCENE_OVERLAY_GIZMO_ID_LAST;
}

RUID RServerObj::pickid_to_ruid(uint32_t pickID)
{
    // reserved SceneOverlayGizmoID
    if (pickID <= SCENE_OVERLAY_GIZMO_ID_LAST)
        return 0;

    return pickID - SCENE_OVERLAY_GIZMO_ID_LAST;
}

uint32_t RServerObj::ruid_to_pickid(RUID ruid)
{
    // NOTE: this should not cause an u32 overflow for counter-based RUID,
    //       but the possibility isn't zero either.
    return ruid + SCENE_OVERLAY_GIZMO_ID_LAST;
}

RServer RServer::create(const RServerInfo& serverI)
{
    LD_ASSERT(serverI.cubemapFaces);

    RServerObj* obj = heap_new<RServerObj>(MEMORY_USAGE_RENDER, serverI);

    return {obj};
}

void RServer::destroy(RServer service)
{
    RServerObj* obj = service;

    heap_delete<RServerObj>(obj);
}

void RServer::next_frame(const RServerFrameInfo& frameI)
{
    LD_ASSERT(frameI.mainCamera);
    LD_ASSERT(frameI.screenExtent.x > 0 && frameI.screenExtent.y > 0);

    mObj->next_frame(frameI);
}

void RServer::submit_frame()
{
    mObj->submit_frame();
}

void RServer::scene_pass(const RServerScenePass& sceneP)
{
    mObj->scene_pass(sceneP);
}

void RServer::editor_pass(const RServerEditorPass& editorRP)
{
    mObj->editor_pass(editorRP);
}

void RServer::editor_overlay_pass(const RServerEditorOverlayPass& editorOP)
{
    mObj->editor_overlay_pass(editorOP);
}

RDevice RServer::get_device()
{
    return mObj->mDevice;
}

RImage RServer::get_font_atlas_image()
{
    return mObj->mFontAtlasImage;
}

RUID RServer::create_mesh(ModelBinary& modelBinary)
{
    RStager stager(mObj->mDevice, RQUEUE_TYPE_GRAPHICS);

    RUID ruid = mObj->mRUIDCtr++;
    RMesh* rmesh = heap_new<RMesh>(MEMORY_USAGE_RENDER);
    rmesh->create_from_binary(mObj->mDevice, stager, modelBinary);
    mObj->mMeshes[ruid] = rmesh;

    stager.submit(mObj->mDevice.get_graphics_queue());

    return ruid;
}

} // namespace LD