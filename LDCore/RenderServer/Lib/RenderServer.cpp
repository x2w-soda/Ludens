#include <Ludens/DSA/HashSet.h>
#include <Ludens/DSA/IDCounter.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Log/Log.h>
#include <Ludens/Memory/Memory.h>
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
#include <Ludens/RenderServer/RenderServer.h>

namespace LD {

static Log sLog("RServer");

struct RMeshEntry
{
    RMesh mesh;              /// mesh resources
    RUID meshID;             /// mesh identifier
    HashSet<RUID> drawCalls; /// draw calls using this mesh
};

/// @brief Render server implementation.
class RenderServerObj
{
    friend class RenderServer;

public:
    RenderServerObj(const RenderServerInfo& serviceI);
    ~RenderServerObj();

    void next_frame(const RenderServerFrameInfo& frameI);
    void submit_frame();

    void scene_pass(const RenderServerScenePass& sceneP);
    void screen_pass(const RenderServerScreenPass& screenP);
    void editor_pass(const RenderServerEditorPass& editorP);
    void editor_overlay_pass(const RenderServerEditorOverlayPass& editorOP);
    void editor_dialog_pass(const RenderServerEditorDialogPass& dialogPass);

    inline RUID get_ruid() { return mRUIDCtr.get_id(); }

private:
    struct Frame
    {
        RBuffer ubo;
        RSet frameSet;
    };

    static void forward_rendering(ForwardRenderComponent renderer, void* user);
    static void screen_rendering(ScreenRenderComponent renderer, void* user);

    bool pickid_is_gizmo(uint32_t pickID);
    RUID pickid_to_ruid(uint32_t pickID);
    uint32_t ruid_to_pickid(RUID ruid);

private:
    RDevice mDevice;
    RGraph mGraph;
    RSetPool mFrameSetPool;
    RImage mFontAtlasImage;
    RImage mWhiteCubemap;
    Camera mMainCamera;
    RMeshBlinnPhongPipeline mMeshPipeline;
    IDCounter<RUID> mRUIDCtr;
    RenderServerTransformCallback mTransformCallback = nullptr;
    void* mTransformCallbackUser = nullptr;
    RenderServerScreenPassLayerCallback mScreenPassLayerCallback = nullptr;
    RenderServerScreenPassCallback mScreenPassCallback = nullptr;
    void* mScreenPassCallbackUser = nullptr;
    Vec2 mSceneExtent;
    Vec2 mScreenExtent;
    Vector<Frame> mFrames;
    Vector<RCommandPool> mCmdPools;
    Vector<RCommandList> mCmdLists;
    std::unordered_map<RUID, RImage> mCubemaps;
    std::unordered_map<RUID, RMeshEntry*> mMeshes;  // TODO: optimize later
    std::unordered_map<RUID, RUID> mDrawCallToMesh; /// map draw call to mesh ID
    RFormat mDepthStencilFormat;                    /// default depth stencil format
    RFormat mColorFormat;                           /// default color format
    RSampleCountBit mMSAA;                          /// number of samples during MSAA, if enabled
    RUID mSceneOutlineSubject;                      /// subject to be outlined in scene render pass
    uint32_t mFramesInFlight = 0;                   /// number of frames in flight
    uint32_t mFrameIndex = 0;                       /// [0, mFramesInFlight)
    FontAtlas mFontAtlas{};                         /// default font atlas for text rendering
    RGraphImage mLastColorAttachment{};             /// last color attachment output
    RGraphImage mLastIDFlagsAttachment{};           /// last scene ID flags attachment output
    bool mHasRenderedScene = false;
};

RenderServerObj::RenderServerObj(const RenderServerInfo& serverI)
    : mDevice(serverI.device), mColorFormat(RFORMAT_RGBA8), mFontAtlas(serverI.fontAtlas)
{
    RSampleCountBit supportedMSCount = mDevice.get_max_sample_count();
    mMSAA = supportedMSCount >= RSAMPLE_COUNT_4_BIT ? RSAMPLE_COUNT_4_BIT : supportedMSCount;
    sLog.info("msaa {} bits suported, using {} sample bits", (int)supportedMSCount, (int)mMSAA);

    uint32_t count;
    RFormat depthStencilFormats[8];
    mDevice.get_depth_stencil_formats(depthStencilFormats, count);
    mDepthStencilFormat = depthStencilFormats[0];

    //
    // Render Server Resources
    //

    Bitmap atlasBitmap = mFontAtlas.get_bitmap();
    RImageInfo imageI = RUtil::make_2d_image_info(RIMAGE_USAGE_SAMPLED_BIT | RIMAGE_USAGE_TRANSFER_DST_BIT, RFORMAT_R8, atlasBitmap.width(), atlasBitmap.height());
    imageI.sampler = {RFILTER_LINEAR, RFILTER_LINEAR, RSAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE};
    mFontAtlasImage = mDevice.create_image(imageI);

    imageI.sampler = {RFILTER_LINEAR, RFILTER_LINEAR, RSAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE};
    imageI = RUtil::make_cube_image_info(RIMAGE_USAGE_SAMPLED_BIT | RIMAGE_USAGE_TRANSFER_DST_BIT, RFORMAT_RGBA8, 1, imageI.sampler);
    mWhiteCubemap = mDevice.create_image(imageI);

    RStager stager(mDevice, RQUEUE_TYPE_GRAPHICS);
    stager.add_image_data(mFontAtlasImage, atlasBitmap.data(), RIMAGE_LAYOUT_SHADER_READ_ONLY);

    const uint32_t whitePixel = 0xFFFFFFFF;
    const uint32_t* whiteFaces[6] = {&whitePixel, &whitePixel, &whitePixel, &whitePixel, &whitePixel, &whitePixel};
    constexpr uint32_t faceSize = 1;
    Bitmap whiteCubemapBitmap = Bitmap::create_cubemap_from_data(faceSize, (const void**)whiteFaces);
    stager.add_image_data(mWhiteCubemap, whiteCubemapBitmap.data(), RIMAGE_LAYOUT_SHADER_READ_ONLY);
    Bitmap::destroy(whiteCubemapBitmap);

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
        RSetImageUpdateInfo imageUpdateI = RUtil::make_single_set_image_update_info(frame.frameSet, 1, RBINDING_TYPE_COMBINED_IMAGE_SAMPLER, &layout, &mWhiteCubemap);
        mDevice.update_set_images(1, &imageUpdateI);
    }
}

RenderServerObj::~RenderServerObj()
{
    mDevice.wait_idle();

    RGraph::release(mDevice);

    for (auto ite : mMeshes)
    {
        RMeshEntry* entry = ite.second;
        entry->mesh.destroy();
        heap_delete<RMeshEntry>(entry);
    }
    mMeshes.clear();

    for (auto ite : mCubemaps)
    {
        RImage cubemap = ite.second;
        mDevice.destroy_image(cubemap);
    }
    mCubemaps.clear();

    for (uint32_t i = 0; i < mFramesInFlight; i++)
    {
        Frame& frame = mFrames[i];
        frame.ubo.unmap();
        mDevice.destroy_buffer(frame.ubo);
        mDevice.destroy_command_pool(mCmdPools[i]);
    }

    mDevice.destroy_set_pool(mFrameSetPool);

    RMeshBlinnPhongPipeline::destroy(mMeshPipeline);

    mDevice.destroy_image(mWhiteCubemap);
    mDevice.destroy_image(mFontAtlasImage);
}

void RenderServerObj::next_frame(const RenderServerFrameInfo& frameI)
{
    RFence frameComplete;
    mDevice.next_frame(mFrameIndex, frameComplete);

    WindowRegistry reg = WindowRegistry::get();
    WindowID rootWindowID = reg.get_root_id();
    Vector<RGraphSwapchainInfo> swapchains;

    {
        RGraphSwapchainInfo rootWindowSwapchain{};
        rootWindowSwapchain.image = mDevice.try_acquire_image(rootWindowID, rootWindowSwapchain.imageAcquired, rootWindowSwapchain.presentReady);
        rootWindowSwapchain.window = rootWindowID;

        if (rootWindowSwapchain.image)
            swapchains.push_back(rootWindowSwapchain);
    }

    if (frameI.dialogWindowID)
    {
        RGraphSwapchainInfo dialogWindowSwapchain{};
        dialogWindowSwapchain.image = mDevice.try_acquire_image(frameI.dialogWindowID, dialogWindowSwapchain.imageAcquired, dialogWindowSwapchain.presentReady);
        dialogWindowSwapchain.window = frameI.dialogWindowID;
        if (dialogWindowSwapchain.image)
            swapchains.push_back(dialogWindowSwapchain);
    }

    mSceneExtent = frameI.sceneExtent;
    mScreenExtent = frameI.screenExtent;
    mCmdPools[mFrameIndex].reset();
    RCommandList list = mCmdLists[mFrameIndex];
    Frame& frame = mFrames[mFrameIndex];

    RGraphInfo graphI{};
    graphI.device = mDevice;
    graphI.list = list;
    graphI.frameComplete = frameComplete;
    graphI.swapchainCount = (uint32_t)swapchains.size();
    graphI.swapchains = swapchains.data();
    graphI.screenWidth = (uint32_t)mScreenExtent.x;
    graphI.screenHeight = (uint32_t)mScreenExtent.y;
    graphI.prePassCB = [](RCommandList list, void* user) {
        auto* obj = (RenderServerObj*)user;
        Frame& frame = obj->mFrames[obj->mFrameIndex];
        list.cmd_bind_graphics_sets(sRMeshPipelineLayout, 0, 1, &frame.frameSet);
    };
    graphI.user = this;
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

    if (mCubemaps.contains(frameI.envCubemap))
    {
        RImage envCubemap = mCubemaps[frameI.envCubemap];
        RImageLayout layout = RIMAGE_LAYOUT_SHADER_READ_ONLY;
        RSetImageUpdateInfo imageUpdateI = RUtil::make_single_set_image_update_info(frame.frameSet, 1, RBINDING_TYPE_COMBINED_IMAGE_SAMPLER, &layout, &envCubemap);
        mDevice.update_set_images(1, &imageUpdateI);
    }

    //
    // initialization
    //

    mLastColorAttachment = {};
    mLastIDFlagsAttachment = {};
    mHasRenderedScene = false;
}

void RenderServerObj::submit_frame()
{
    WindowID rootID = WindowRegistry::get().get_root_id();

    // blit to swapchain images and submit
    mGraph.connect_swapchain_image(mLastColorAttachment, rootID);
    mGraph.submit();
    RGraph::destroy(mGraph);

    mDevice.present_frame();
}

void RenderServerObj::scene_pass(const RenderServerScenePass& sceneP)
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
    forwardI.hasSkybox = sceneP.hasSkybox;
    ForwardRenderComponent sceneFR = ForwardRenderComponent::add(mGraph, forwardI, &RenderServerObj::forward_rendering, this);

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
        mGraph.connect_image(sceneFR.out_color_attachment(), overlayC.in_color_attachment());
        mGraph.connect_image(sceneFR.out_id_flags_attachment(), overlayC.in_id_flags_attachment());
        mLastColorAttachment = overlayC.out_color_attachment();
        mLastIDFlagsAttachment = overlayC.out_id_flags_attachment();
    }
    else
    {
        mLastColorAttachment = sceneFR.out_color_attachment();
        mLastIDFlagsAttachment = sceneFR.out_id_flags_attachment();
    }

    mHasRenderedScene = true;
}

void RenderServerObj::screen_pass(const RenderServerScreenPass& screenP)
{
    LD_ASSERT(mHasRenderedScene);

    mScreenPassLayerCallback = screenP.layerCallback;
    mScreenPassCallback = screenP.callback;
    mScreenPassCallbackUser = screenP.user;

    ScreenRenderComponentInfo screenRCI{};
    screenRCI.format = mColorFormat;
    screenRCI.onDrawCallback = &RenderServerObj::screen_rendering;
    screenRCI.user = this;
    screenRCI.hasInputImage = true; // draws on top of the scene_pass results
    screenRCI.hasSampledImage = false;
    screenRCI.name = "SceneScreen";
    screenRCI.screenExtent = &mSceneExtent; // scene extent is typically smaller than screen extent in editor
    ScreenRenderComponent screenRC = ScreenRenderComponent::add(mGraph, screenRCI);
    mGraph.connect_image(mLastColorAttachment, screenRC.color_attachment());
    mLastColorAttachment = screenRC.color_attachment();
}

void RenderServerObj::editor_pass(const RenderServerEditorPass& editorP)
{
    LD_ASSERT(mHasRenderedScene && mLastColorAttachment && mLastIDFlagsAttachment);

    ScreenRenderComponentInfo screenRCI{};
    screenRCI.format = mColorFormat;
    screenRCI.onDrawCallback = editorP.renderCallback;
    screenRCI.user = editorP.user;
    screenRCI.hasInputImage = false;
    screenRCI.hasSampledImage = true;
    screenRCI.clearColor = 0x000000FF;
    screenRCI.name = "Editor";
    ScreenRenderComponent editorSRC = ScreenRenderComponent::add(mGraph, screenRCI);
    mGraph.connect_image(mLastColorAttachment, editorSRC.sampled_attachment());
    mLastColorAttachment = editorSRC.color_attachment();

    ScreenPickComponentInfo pickCI{};
    pickCI.pickQueryCount = 0;
    if (editorP.sceneMousePickQuery)
    {
        pickCI.pickQueryCount = 1;
        pickCI.pickPositions = editorP.sceneMousePickQuery;
    }
    ScreenPickComponent screenPick = ScreenPickComponent::add(mGraph, pickCI);
    mGraph.connect_image(mLastIDFlagsAttachment, screenPick.attachment());

    // NOTE: The results are actually from mFramesInFlight frames ago,
    //       stalling the GPU just to acquire results in the same frame
    //       would be terrible for CPU-GPU concurrency.
    //       See ScreenPickComponent implementation.
    Vector<ScreenPickResult> pickResults;
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

void RenderServerObj::editor_overlay_pass(const RenderServerEditorOverlayPass& editorOP)
{
    /*
    DualKawaseComponentInfo blurCI{};
    blurCI.format = mColorFormat;
    blurCI.mixColor = editorOP.blurMixColor;
    blurCI.mixFactor = editorOP.blurMixFactor;
    DualKawaseComponent blurC = DualKawaseComponent::add(mGraph, blurCI);
    mGraph.connect_image(mLastComponent, mLastColorAttachment, blurC.component_name(), blurC.input_name());
    */

    ScreenRenderComponentInfo screenRCI{};
    screenRCI.format = mColorFormat;
    screenRCI.onDrawCallback = editorOP.renderCallback;
    screenRCI.user = editorOP.user;
    screenRCI.hasInputImage = true;
    screenRCI.hasSampledImage = false;
    screenRCI.name = "EditorOverlay";
    ScreenRenderComponent editorSRC = ScreenRenderComponent::add(mGraph, screenRCI);
    mGraph.connect_image(mLastColorAttachment, editorSRC.color_attachment());
    // mGraph.connect_image(blurC.component_name(), blurC.output_name(), editorSRC.component_name(), editorSRC.sampled_name());

    mLastColorAttachment = editorSRC.color_attachment();
}

void RenderServerObj::editor_dialog_pass(const RenderServerEditorDialogPass& editorDP)
{
    ScreenRenderComponentInfo screenRCI{};
    screenRCI.format = mColorFormat;
    screenRCI.onDrawCallback = editorDP.renderCallback;
    screenRCI.user = editorDP.user;
    screenRCI.hasInputImage = false;
    screenRCI.hasSampledImage = false;
    screenRCI.name = "EditorDialog";
    ScreenRenderComponent editorSRC = ScreenRenderComponent::add(mGraph, screenRCI);
    mGraph.connect_swapchain_image(editorSRC.color_attachment(), editorDP.dialogWindow);
}

// NOTE: This is super early placeholder scene renderer implementation.
//       Once other engine subsystems such as Assets and Scenes are resolved,
//       we will come back and replace this silly procedure.
void RenderServerObj::forward_rendering(ForwardRenderComponent renderer, void* user)
{
    LD_PROFILE_SCOPE;

    RenderServerObj& self = *(RenderServerObj*)user;
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
        RMeshEntry* entry = ite.second;

        for (RUID drawCall : entry->drawCalls)
        {
            pc.model = self.mTransformCallback(drawCall, self.mTransformCallbackUser);
            pc.id = self.ruid_to_pickid(drawCall);
            pc.flags = 0;

            renderer.set_push_constant(sRMeshPipelineLayout, 0, sizeof(pc), &pc);
            renderer.draw_mesh(entry->mesh);
        }
    }

    // render flag hints for object outlining
    RUID outlineDrawCall = self.mSceneOutlineSubject;
    if (outlineDrawCall != 0 && self.mDrawCallToMesh.contains(outlineDrawCall))
    {
        RUID meshID = self.mDrawCallToMesh[outlineDrawCall];
        LD_ASSERT(self.mMeshes.contains(meshID));
        RMeshEntry* entry = self.mMeshes[meshID];

        // render to 16-bit flags only
        meshPipeline.set_color_write_mask(0, 0);
        meshPipeline.set_color_write_mask(1, RCOLOR_COMPONENT_B_BIT | RCOLOR_COMPONENT_A_BIT);
        meshPipeline.set_depth_test_enable(false);

        pc.model = self.mTransformCallback(outlineDrawCall, self.mTransformCallbackUser);
        pc.id = 0;    // not written to color attachment due to write masks
        pc.flags = 1; // currently any non-zero flag value indicates mesh that requires outlining

        renderer.set_push_constant(sRMeshPipelineLayout, 0, sizeof(pc), &pc);
        renderer.draw_mesh(entry->mesh);
    }

    renderer.draw_skybox();
}

void RenderServerObj::screen_rendering(ScreenRenderComponent renderer, void* user)
{
    LD_PROFILE_SCOPE;
    RenderServerObj& self = *(RenderServerObj*)user;

    if (self.mScreenPassLayerCallback)
    {
        // ask server user for ScreenLayer to render
        ScreenLayer layer = self.mScreenPassLayerCallback(self.mScreenPassCallbackUser);

        if (layer)
        {
            Vector<ScreenLayerItem> drawList = layer.get_draw_list();

            for (const ScreenLayerItem& item : drawList)
            {
                renderer.draw(item.tl, item.tr, item.br, item.bl, item.image, item.color);
            }
        }
    }

    if (self.mScreenPassCallback)
    {
        self.mScreenPassCallback(renderer, self.mScreenPassCallbackUser);
    }
}

bool RenderServerObj::pickid_is_gizmo(uint32_t pickID)
{
    return 1 <= pickID && pickID <= SCENE_OVERLAY_GIZMO_ID_LAST;
}

RUID RenderServerObj::pickid_to_ruid(uint32_t pickID)
{
    // reserved SceneOverlayGizmoID
    if (pickID <= SCENE_OVERLAY_GIZMO_ID_LAST)
        return 0;

    return pickID - SCENE_OVERLAY_GIZMO_ID_LAST;
}

uint32_t RenderServerObj::ruid_to_pickid(RUID ruid)
{
    // NOTE: this should not cause an u32 overflow for counter-based RUID,
    //       but the possibility isn't zero either.
    return ruid + SCENE_OVERLAY_GIZMO_ID_LAST;
}

RenderServer RenderServer::create(const RenderServerInfo& serverI)
{
    RenderServerObj* obj = heap_new<RenderServerObj>(MEMORY_USAGE_RENDER, serverI);

    return {obj};
}

void RenderServer::destroy(RenderServer service)
{
    RenderServerObj* obj = service;

    heap_delete<RenderServerObj>(obj);
}

void RenderServer::next_frame(const RenderServerFrameInfo& frameI)
{
    LD_ASSERT(frameI.mainCamera);
    LD_ASSERT(frameI.screenExtent.x > 0 && frameI.screenExtent.y > 0);

    mObj->next_frame(frameI);
}

void RenderServer::submit_frame()
{
    mObj->submit_frame();
}

void RenderServer::scene_pass(const RenderServerScenePass& sceneP)
{
    mObj->scene_pass(sceneP);
}

void RenderServer::screen_pass(const RenderServerScreenPass& screenP)
{
    mObj->screen_pass(screenP);
}

void RenderServer::editor_pass(const RenderServerEditorPass& editorRP)
{
    mObj->editor_pass(editorRP);
}

void RenderServer::editor_overlay_pass(const RenderServerEditorOverlayPass& editorOP)
{
    mObj->editor_overlay_pass(editorOP);
}

void RenderServer::editor_dialog_pass(const RenderServerEditorDialogPass& dialogPass)
{
    mObj->editor_dialog_pass(dialogPass);
}

RDevice RenderServer::get_device()
{
    return mObj->mDevice;
}

RImage RenderServer::get_font_atlas_image()
{
    return mObj->mFontAtlasImage;
}

bool RenderServer::mesh_exists(RUID mesh)
{
    return mObj->mMeshes.contains(mesh);
}

RUID RenderServer::create_mesh(ModelBinary& modelBinary)
{
    RStager stager(mObj->mDevice, RQUEUE_TYPE_GRAPHICS);

    RUID meshID = mObj->get_ruid();
    RMeshEntry* entry = heap_new<RMeshEntry>(MEMORY_USAGE_RENDER);
    mObj->mMeshes[meshID] = entry;

    entry->mesh.create_from_binary(mObj->mDevice, stager, modelBinary);
    entry->meshID = meshID;
    stager.submit(mObj->mDevice.get_graphics_queue());

    return meshID;
}

RUID RenderServer::create_mesh_draw_call(RUID meshID)
{
    auto ite = mObj->mMeshes.find(meshID);

    if (ite == mObj->mMeshes.end())
        return 0;

    RMeshEntry* entry = mObj->mMeshes[meshID];
    RUID drawCall = mObj->get_ruid();
    entry->drawCalls.insert(drawCall);
    mObj->mDrawCallToMesh[drawCall] = meshID;

    return drawCall;
}

void RenderServer::destroy_mesh_draw_call(RUID drawCall)
{
    auto ite = mObj->mDrawCallToMesh.find(drawCall);

    if (ite == mObj->mDrawCallToMesh.end())
        return;

    RUID meshID = mObj->mDrawCallToMesh[drawCall];
    RMeshEntry* entry = mObj->mMeshes[meshID];

    entry->drawCalls.erase(drawCall);
}

RUID RenderServer::create_cubemap(Bitmap cubemapFaces)
{
    RSamplerInfo cubemapSamplerI{};
    cubemapSamplerI.filter = RFILTER_LINEAR;
    cubemapSamplerI.mipmapFilter = RFILTER_LINEAR;
    cubemapSamplerI.addressMode = RSAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

    RDevice device = mObj->mDevice;
    RImageInfo imageI = RUtil::make_cube_image_info(RIMAGE_USAGE_SAMPLED_BIT | RIMAGE_USAGE_TRANSFER_DST_BIT, RFORMAT_RGBA8, cubemapFaces.width(), cubemapSamplerI);
    RImage cubemap = device.create_image(imageI);
    RStager stager(device, RQUEUE_TYPE_GRAPHICS);
    stager.add_image_data(cubemap, cubemapFaces.data(), RIMAGE_LAYOUT_SHADER_READ_ONLY);
    stager.submit(device.get_graphics_queue());

    RUID cubemapID = mObj->get_ruid();
    mObj->mCubemaps[cubemapID] = cubemap;

    return cubemapID;
}

void RenderServer::destroy_cubemap(RUID cubemapID)
{
    auto ite = mObj->mCubemaps.find(cubemapID);

    if (ite == mObj->mCubemaps.end())
        return;

    RImage cubemap = ite->second;
    mObj->mCubemaps.erase(ite);

    mObj->mDevice.wait_idle();
    mObj->mDevice.destroy_image(cubemap);
}

} // namespace LD
