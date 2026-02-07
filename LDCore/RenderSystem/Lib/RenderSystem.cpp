#include <Ludens/DSA/HashMap.h>
#include <Ludens/DSA/HashSet.h>
#include <Ludens/DSA/IDCounter.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Log/Log.h>
#include <Ludens/Memory/Allocator.h>
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
#include <Ludens/RenderSystem/RenderSystem.h>

#include "ScreenLayer.h"

namespace LD {

static Log sLog("RenderSystem");

/// @brief Render system implementation.
class RenderSystemObj
{
public:
    RenderSystemObj(const RenderSystemInfo& serviceI);
    ~RenderSystemObj();

    void next_frame(const RenderSystemFrameInfo& frameI);
    void submit_frame();

    void scene_pass(const RenderSystemScenePass& sceneP);
    void screen_pass(const RenderSystemScreenPass& screenP);
    void editor_pass(const RenderSystemEditorPass& editorP);
    void editor_overlay_pass(const RenderSystemEditorOverlayPass& editorOP);
    void editor_dialog_pass(const RenderSystemEditorDialogPass& dialogPass);

    RUID create_screen_layer(const std::string& name);
    void destroy_screen_layer(RUID layerID);

    RImage create_image_2d(Bitmap bitmap);
    void destroy_image_2d(RImage image);

    RImage create_image_cube(Bitmap cubemapFaces);
    void destroy_image_cube(RImage image);

    MeshDataObj* create_mesh_data(ModelBinary& binary);
    void destroy_mesh_data(MeshDataObj* data);

    MeshDrawObj* create_mesh_draw(MeshDataObj* data);
    void destroy_mesh_draw(MeshDrawObj* draw);

    Sprite2DDrawObj* create_sprite_2d_draw(RImage image, RUID layerID, const Rect& rect, uint32_t zDepth);
    void destroy_sprite_2d_draw(Sprite2DDrawObj* draw);

    inline RUID get_id() { return mRUIDCtr.get_id(); }
    inline RImage get_font_atlas_image() { return mFontAtlasImage; }

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

private: // render passes and pipelines
    RDevice mDevice;
    RGraph mGraph;
    RSetPool mFrameSetPool;
    RImage mFontAtlasImage;
    RImage mWhiteCubemap;
    Camera mMainCamera;
    RMeshBlinnPhongPipeline mMeshPipeline;
    RenderSystemMat4Callback mScenePassMat4Callback = nullptr;
    void* mScenePassUser = nullptr;
    RenderSystemMat4Callback mScreenPassMat4Callback = nullptr;
    RenderSystemScreenPassCallback mScreenPassCallback = nullptr;
    void* mScreenPassUser = nullptr;
    Vec2 mSceneExtent;
    Vec2 mScreenExtent;
    Vec4 mClearColor;
    Vector<Frame> mFrames;
    Vector<RCommandPool> mCmdPools;
    Vector<RCommandList> mCmdLists;
    uint32_t mFramesInFlight = 0;         /// number of frames in flight
    uint32_t mFrameIndex = 0;             /// [0, mFramesInFlight)
    FontAtlas mFontAtlas{};               /// default font atlas for text rendering
    RGraphImage mLastColorAttachment{};   /// last color attachment output
    RGraphImage mLastIDFlagsAttachment{}; /// last scene ID flags attachment output
    RFormat mDepthStencilFormat;          /// default depth stencil format
    RFormat mColorFormat;                 /// default color format
    RSampleCountBit mMSAA;                /// number of samples during MSAA, if enabled
    RUID mSceneOutlineSubject;            /// subject to be outlined in scene render pass
    bool mHasAcquiredRootWindowImage = false;
    bool mHasAcquiredDialogWindowImage = false;

private:
    IDCounter<RUID> mRUIDCtr;
    HashMap<RUID, ScreenLayerObj*> mLayers;
    HashMap<RUID, RImage> mImages;
    HashMap<RUID, MeshDataObj*> mMeshData;
    HashMap<RUID, MeshDrawObj*> mMeshDraw;         /// Mesh draw info
    HashMap<RUID, Sprite2DDrawObj*> mSprite2DDraw; /// Spirte2D draw info
    PoolAllocator mMeshDataPA{};
    PoolAllocator mMeshDrawPA{};
};

RenderSystemObj::RenderSystemObj(const RenderSystemInfo& systemI)
    : mDevice(systemI.device), mColorFormat(RFORMAT_RGBA8), mFontAtlas(systemI.fontAtlas)
{
    LD_PROFILE_SCOPE;

    RSampleCountBit supportedMSCount = mDevice.get_max_sample_count();
    mMSAA = supportedMSCount >= RSAMPLE_COUNT_4_BIT ? RSAMPLE_COUNT_4_BIT : supportedMSCount;
    sLog.info("msaa {} bits suported, using {} sample bits", (int)supportedMSCount, (int)mMSAA);

    uint32_t count;
    RFormat depthStencilFormats[8];
    mDevice.get_depth_stencil_formats(depthStencilFormats, count);
    mDepthStencilFormat = depthStencilFormats[0];

    //
    // Render System Resources
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

    //
    // User created resources
    //

    PoolAllocatorInfo paI{};
    paI.usage = MEMORY_USAGE_RENDER;
    paI.isMultiPage = true;
    paI.blockSize = sizeof(MeshDrawObj);
    paI.pageSize = 128;
    mMeshDrawPA = PoolAllocator::create(paI);

    paI.blockSize = sizeof(MeshDataObj);
    paI.pageSize = 256;
    mMeshDataPA = PoolAllocator::create(paI);
}

RenderSystemObj::~RenderSystemObj()
{
    LD_PROFILE_SCOPE;

    mDevice.wait_idle();

    for (auto it = mMeshDataPA.begin(); it; ++it)
    {
        auto* data = (MeshDataObj*)it.data();
        data->id = 0;
        data->~MeshDataObj();
    }
    PoolAllocator::destroy(mMeshDataPA);

    for (auto it = mMeshDrawPA.begin(); it; ++it)
    {
        auto* draw = (MeshDrawObj*)it.data();
        draw->id = 0;
        draw->~MeshDrawObj();
    }
    PoolAllocator::destroy(mMeshDrawPA);

    RGraph::release(mDevice);

    for (auto it : mLayers)
        heap_delete<ScreenLayerObj>(it.second);

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

void RenderSystemObj::next_frame(const RenderSystemFrameInfo& frameI)
{
    RFence frameComplete;
    mDevice.next_frame(mFrameIndex, frameComplete);
    mClearColor = frameI.clearColor;

    WindowRegistry reg = WindowRegistry::get();
    WindowID rootWindowID = reg.get_root_id();
    Vector<RGraphSwapchainInfo> swapchains;

    mHasAcquiredRootWindowImage = false;
    mHasAcquiredDialogWindowImage = false;

    {
        RGraphSwapchainInfo rootWindowSwapchain{};
        rootWindowSwapchain.image = mDevice.try_acquire_image(rootWindowID, rootWindowSwapchain.imageAcquired, rootWindowSwapchain.presentReady);
        rootWindowSwapchain.window = rootWindowID;

        if (rootWindowSwapchain.image)
        {
            swapchains.push_back(rootWindowSwapchain);
            mHasAcquiredRootWindowImage = true;
        }
    }

    if (frameI.dialogWindowID)
    {
        RGraphSwapchainInfo dialogWindowSwapchain{};
        dialogWindowSwapchain.image = mDevice.try_acquire_image(frameI.dialogWindowID, dialogWindowSwapchain.imageAcquired, dialogWindowSwapchain.presentReady);
        dialogWindowSwapchain.window = frameI.dialogWindowID;
        if (dialogWindowSwapchain.image)
        {
            swapchains.push_back(dialogWindowSwapchain);
            mHasAcquiredDialogWindowImage = true;
        }
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
        auto* obj = (RenderSystemObj*)user;
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

    if (mImages.contains(frameI.envCubemap))
    {
        RImage envCubemap = mImages[frameI.envCubemap];
        RImageLayout layout = RIMAGE_LAYOUT_SHADER_READ_ONLY;
        RSetImageUpdateInfo imageUpdateI = RUtil::make_single_set_image_update_info(frame.frameSet, 1, RBINDING_TYPE_COMBINED_IMAGE_SAMPLER, &layout, &envCubemap);
        mDevice.update_set_images(1, &imageUpdateI);
    }

    //
    // initialization
    //

    mLastColorAttachment = {};
    mLastIDFlagsAttachment = {};
}

void RenderSystemObj::submit_frame()
{
    LD_PROFILE_SCOPE;

    WindowID rootID = WindowRegistry::get().get_root_id();

    if (mHasAcquiredRootWindowImage)
    {
        // blit to root window swapchain image and submit
        mGraph.connect_swapchain_image(mLastColorAttachment, rootID);
    }

    mGraph.submit();
    RGraph::destroy(mGraph);

    mDevice.present_frame();
}

void RenderSystemObj::scene_pass(const RenderSystemScenePass& sceneP)
{
    LD_PROFILE_SCOPE;

    if (!mHasAcquiredRootWindowImage)
        return;

    RClearDepthStencilValue clearDS = {.depth = 1.0f, .stencil = 0};

    mSceneOutlineSubject = sceneP.overlay.enabled ? sceneP.overlay.outlineRUID : 0;
    mScenePassMat4Callback = sceneP.mat4Callback;
    mScenePassUser = sceneP.user;

    ForwardRenderComponentInfo forwardI{};
    forwardI.width = (uint32_t)mSceneExtent.x;
    forwardI.height = (uint32_t)mSceneExtent.y;
    forwardI.colorFormat = mColorFormat;
    forwardI.clearColor = RUtil::make_clear_color(mClearColor.r, mClearColor.g, mClearColor.b, mClearColor.a);
    forwardI.depthStencilFormat = mDepthStencilFormat;
    forwardI.clearDepthStencil = clearDS;
    forwardI.samples = mMSAA;
    forwardI.hasSkybox = sceneP.hasSkybox;
    ForwardRenderComponent sceneFR = ForwardRenderComponent::add(mGraph, forwardI, &RenderSystemObj::forward_rendering, this);

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
}

void RenderSystemObj::screen_pass(const RenderSystemScreenPass& screenP)
{
    LD_PROFILE_SCOPE;

    if (!mHasAcquiredRootWindowImage)
        return;

    mScreenPassMat4Callback = screenP.mat4Callback;
    mScreenPassCallback = screenP.callback;
    mScreenPassUser = screenP.user;

    ScreenRenderComponentInfo screenRCI{};
    screenRCI.format = mColorFormat;
    screenRCI.onDrawCallback = &RenderSystemObj::screen_rendering;
    screenRCI.user = this;
    screenRCI.hasSampledImage = false;
    screenRCI.name = "SceneScreen";
    screenRCI.screenExtent = &mSceneExtent; // scene extent is typically smaller than screen extent in editor

    if (mLastColorAttachment)
    {
        screenRCI.hasInputImage = true; // draws on top of the scene_pass results
        ScreenRenderComponent screenRC = ScreenRenderComponent::add(mGraph, screenRCI);
        mGraph.connect_image(mLastColorAttachment, screenRC.color_attachment());
        mLastColorAttachment = screenRC.color_attachment();
    }
    else
    {
        screenRCI.hasInputImage = false;
        screenRCI.clearColor = Color(mClearColor); // NOTE: this drops precision from Vec4 to Color (u32)
        ScreenRenderComponent screenRC = ScreenRenderComponent::add(mGraph, screenRCI);
        mLastColorAttachment = screenRC.color_attachment();
    }
}

void RenderSystemObj::editor_pass(const RenderSystemEditorPass& editorP)
{
    LD_PROFILE_SCOPE;

    if (!mHasAcquiredRootWindowImage)
        return;

    LD_ASSERT(mLastColorAttachment);

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

    if (mLastIDFlagsAttachment) // mouse picking in editor
    {
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
}

void RenderSystemObj::editor_overlay_pass(const RenderSystemEditorOverlayPass& editorOP)
{
    LD_PROFILE_SCOPE;

    if (!mHasAcquiredRootWindowImage)
        return;

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

void RenderSystemObj::editor_dialog_pass(const RenderSystemEditorDialogPass& editorDP)
{
    LD_PROFILE_SCOPE;

    if (!mHasAcquiredDialogWindowImage)
        return;

    ScreenRenderComponentInfo screenRCI{};
    screenRCI.format = mColorFormat;
    screenRCI.onDrawCallback = editorDP.renderCallback;
    screenRCI.user = editorDP.user;
    screenRCI.hasInputImage = false;
    screenRCI.hasSampledImage = false;
    screenRCI.name = "EditorDialog";
    screenRCI.screenExtent = nullptr; // TODO:
    ScreenRenderComponent editorSRC = ScreenRenderComponent::add(mGraph, screenRCI);
    mGraph.connect_swapchain_image(editorSRC.color_attachment(), editorDP.dialogWindow);
}

RUID RenderSystemObj::create_screen_layer(const std::string& name)
{
    RUID layerID = get_ruid();

    ScreenLayerObj* obj = mLayers[layerID] = heap_new<ScreenLayerObj>(MEMORY_USAGE_RENDER, layerID, name);

    return layerID;
}

void RenderSystemObj::destroy_screen_layer(RUID layerID)
{
    if (!mLayers.contains(layerID))
        return;

    heap_delete<ScreenLayerObj>(mLayers[layerID]);

    mLayers.erase(layerID);
}

RImage RenderSystemObj::create_image_2d(Bitmap bitmap)
{
    LD_PROFILE_SCOPE;

    RImageInfo imageI = RUtil::make_2d_image_info(RIMAGE_USAGE_SAMPLED_BIT | RIMAGE_USAGE_TRANSFER_DST_BIT, RFORMAT_RGBA8, bitmap.width(), bitmap.height());
    imageI.sampler = {RFILTER_LINEAR, RFILTER_LINEAR, RSAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE};
    RImage image = mDevice.create_image(imageI);

    RStager stager(mDevice, RQUEUE_TYPE_GRAPHICS);
    stager.add_image_data(image, bitmap.data(), RIMAGE_LAYOUT_SHADER_READ_ONLY);
    stager.submit(mDevice.get_graphics_queue());

    mImages[image.get_id()] = image;

    return image;
}

void RenderSystemObj::destroy_image_2d(RImage image)
{
    LD_PROFILE_SCOPE;

    auto it = mImages.find(image.get_id());

    if (it == mImages.end() || it->second.type() != RIMAGE_TYPE_2D)
        return;

    mImages.erase(it);

    mDevice.wait_idle();
    mDevice.destroy_image(image);
}

RImage RenderSystemObj::create_image_cube(Bitmap cubemapFaces)
{
    RSamplerInfo cubemapSamplerI{};
    cubemapSamplerI.filter = RFILTER_LINEAR;
    cubemapSamplerI.mipmapFilter = RFILTER_LINEAR;
    cubemapSamplerI.addressMode = RSAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

    RImageInfo imageI = RUtil::make_cube_image_info(RIMAGE_USAGE_SAMPLED_BIT | RIMAGE_USAGE_TRANSFER_DST_BIT, RFORMAT_RGBA8, cubemapFaces.width(), cubemapSamplerI);
    RImage cubemap = mDevice.create_image(imageI);
    RStager stager(mDevice, RQUEUE_TYPE_GRAPHICS);
    stager.add_image_data(cubemap, cubemapFaces.data(), RIMAGE_LAYOUT_SHADER_READ_ONLY);
    stager.submit(mDevice.get_graphics_queue());

    mImages[cubemap.get_id()] = cubemap;

    return cubemap;
}

void RenderSystemObj::destroy_image_cube(RImage image)
{
    auto it = mImages.find(image.get_id());

    if (it == mImages.end() || it->second.type() != RIMAGE_TYPE_CUBE)
        return;

    mImages.erase(it);

    mDevice.wait_idle();
    mDevice.destroy_image(image);
}

MeshDataObj* RenderSystemObj::create_mesh_data(ModelBinary& binary)
{
    RStager stager(mDevice, RQUEUE_TYPE_GRAPHICS);

    RUID dataID = get_ruid();
    MeshDataObj* dataObj = (MeshDataObj*)mMeshDataPA.allocate();
    new (dataObj) MeshDataObj();
    mMeshData[dataID] = dataObj;

    dataObj->mesh.create_from_binary(mDevice, stager, binary);
    dataObj->id = dataID;
    stager.submit(mDevice.get_graphics_queue());

    return dataObj;
}

void RenderSystemObj::destroy_mesh_data(MeshDataObj* data)
{
    if (!data || !mMeshData.contains(data->id))
        return;

    RUID dataID = data->id;

    mDevice.wait_idle();
    data->mesh.destroy();
    data->~MeshDataObj();
    data->id = 0; // invalidates remaining MeshData handles
    mMeshDataPA.free(data);
    mMeshData.erase(dataID);
}

MeshDrawObj* RenderSystemObj::create_mesh_draw(MeshDataObj* data)
{
    RUID drawID = get_ruid();
    MeshDrawObj* drawObj = (MeshDrawObj*)mMeshDrawPA.allocate();
    new (drawObj) MeshDataObj();
    mMeshDraw[drawID] = drawObj;

    drawObj->id = drawID;

    // NOTE: we allow creating an empty mesh draw without data.
    if (data)
    {
        drawObj->data = MeshData(data, data->id);
        data->drawID.insert(drawID);
    }

    return drawObj;
}

void RenderSystemObj::destroy_mesh_draw(MeshDrawObj* draw)
{
    if (!draw || !mMeshDraw.contains(draw->id))
        return;

    RUID drawID = draw->id;

    mDevice.wait_idle();
    draw->~MeshDrawObj();
    draw->id = 0; // invalidates remaining MeshDraw handles
    mMeshDrawPA.free(draw);
    mMeshDraw.erase(drawID);
}

Sprite2DDrawObj* RenderSystemObj::create_sprite_2d_draw(RImage image, RUID layerID, const Rect& rect, uint32_t zDepth)
{
    LD_ASSERT(image && mLayers.contains(layerID));

    ScreenLayerObj* layer = mLayers[layerID];
    Sprite2DDrawObj* draw = layer->create_sprite_2d(get_ruid(), rect, image, zDepth);

    mSprite2DDraw[draw->id] = draw;

    return draw;
}

void RenderSystemObj::destroy_sprite_2d_draw(Sprite2DDrawObj* draw)
{
    LD_ASSERT(draw && draw->layer);

    draw->layer->destroy_sprite_2d(draw);

    mSprite2DDraw.erase(draw->id);
}

// NOTE: This is super early placeholder scene renderer implementation.
//       Once other engine subsystems such as Assets and Scenes are resolved,
//       we will come back and replace this silly procedure.
void RenderSystemObj::forward_rendering(ForwardRenderComponent renderer, void* user)
{
    LD_PROFILE_SCOPE;

    RenderSystemObj& self = *(RenderSystemObj*)user;
    RPipeline meshPipeline = self.mMeshPipeline.handle();

    if (!self.mHasAcquiredRootWindowImage)
        return;

    renderer.set_mesh_pipeline(meshPipeline);

    // render Color and 16-bit ID
    meshPipeline.set_color_write_mask(0, RCOLOR_COMPONENT_R_BIT | RCOLOR_COMPONENT_G_BIT | RCOLOR_COMPONENT_B_BIT | RCOLOR_COMPONENT_A_BIT);
    meshPipeline.set_color_write_mask(1, RCOLOR_COMPONENT_R_BIT | RCOLOR_COMPONENT_G_BIT);
    meshPipeline.set_depth_test_enable(true);

    RMeshBlinnPhongPipeline::PushConstant pc;

    // render static mesh
    // TODO: iteration can be cache-efficient if MeshData* is allocated from a PoolAllocator
    for (auto ite : self.mMeshData)
    {
        MeshDataObj* data = ite.second;

        for (RUID drawID : data->drawID)
        {
            pc.model = self.mScenePassMat4Callback(drawID, self.mScenePassUser);
            pc.id = self.ruid_to_pickid(drawID);
            pc.flags = 0;

            renderer.set_push_constant(sRMeshPipelineLayout, 0, sizeof(pc), &pc);
            renderer.draw_mesh(data->mesh);
        }
    }

    // render flag hints for object outlining
    RUID outlineDrawID = self.mSceneOutlineSubject;
    if (outlineDrawID != 0 && self.mMeshDraw.contains(outlineDrawID))
    {
        MeshDrawObj* draw = self.mMeshDraw[outlineDrawID];
        LD_ASSERT(draw && draw->data);

        MeshDataObj* data = draw->data.unwrap();

        // render to 16-bit flags only
        meshPipeline.set_color_write_mask(0, 0);
        meshPipeline.set_color_write_mask(1, RCOLOR_COMPONENT_B_BIT | RCOLOR_COMPONENT_A_BIT);
        meshPipeline.set_depth_test_enable(false);

        pc.model = self.mScenePassMat4Callback(outlineDrawID, self.mScenePassUser);
        pc.id = 0;    // not written to color attachment due to write masks
        pc.flags = 1; // currently any non-zero flag value indicates mesh that requires outlining

        renderer.set_push_constant(sRMeshPipelineLayout, 0, sizeof(pc), &pc);
        renderer.draw_mesh(data->mesh);
    }

    renderer.draw_skybox();
}

void RenderSystemObj::screen_rendering(ScreenRenderComponent renderer, void* user)
{
    LD_PROFILE_SCOPE;

    RenderSystemObj& self = *(RenderSystemObj*)user;

    if (!self.mHasAcquiredRootWindowImage)
        return;

    // TODO: layer draw order!
    for (auto it : self.mLayers)
    {
        it.second->invalidate(self.mScreenPassMat4Callback, self.mScreenPassUser);

        TView<ScreenLayerItem> drawList = it.second->get_draw_list();

        for (size_t i = 0; i < drawList.size; i++)
        {
            const ScreenLayerItem& item = drawList.data[i];
            renderer.draw(item.tl, item.tr, item.br, item.bl, item.image, item.color);
        }
    }

    if (self.mScreenPassCallback)
    {
        self.mScreenPassCallback(renderer, self.mScreenPassUser);
    }
}

bool RenderSystemObj::pickid_is_gizmo(uint32_t pickID)
{
    return 1 <= pickID && pickID <= SCENE_OVERLAY_GIZMO_ID_LAST;
}

RUID RenderSystemObj::pickid_to_ruid(uint32_t pickID)
{
    // reserved SceneOverlayGizmoID
    if (pickID <= SCENE_OVERLAY_GIZMO_ID_LAST)
        return 0;

    return pickID - SCENE_OVERLAY_GIZMO_ID_LAST;
}

uint32_t RenderSystemObj::ruid_to_pickid(RUID ruid)
{
    // NOTE: this should not cause an u32 overflow for counter-based RUID,
    //       but the possibility isn't zero either.
    return ruid + SCENE_OVERLAY_GIZMO_ID_LAST;
}

RenderSystem RenderSystem::create(const RenderSystemInfo& systemI)
{
    RenderSystemObj* obj = heap_new<RenderSystemObj>(MEMORY_USAGE_RENDER, systemI);

    return {obj};
}

void RenderSystem::destroy(RenderSystem service)
{
    RenderSystemObj* obj = service;

    heap_delete<RenderSystemObj>(obj);
}

void RenderSystem::next_frame(const RenderSystemFrameInfo& frameI)
{
    LD_ASSERT(frameI.mainCamera);
    LD_ASSERT(frameI.screenExtent.x > 0 && frameI.screenExtent.y > 0);

    mObj->next_frame(frameI);
}

void RenderSystem::submit_frame()
{
    mObj->submit_frame();
}

void RenderSystem::scene_pass(const RenderSystemScenePass& sceneP)
{
    mObj->scene_pass(sceneP);
}

void RenderSystem::screen_pass(const RenderSystemScreenPass& screenP)
{
    mObj->screen_pass(screenP);
}

void RenderSystem::editor_pass(const RenderSystemEditorPass& editorRP)
{
    mObj->editor_pass(editorRP);
}

void RenderSystem::editor_overlay_pass(const RenderSystemEditorOverlayPass& editorOP)
{
    mObj->editor_overlay_pass(editorOP);
}

void RenderSystem::editor_dialog_pass(const RenderSystemEditorDialogPass& dialogPass)
{
    mObj->editor_dialog_pass(dialogPass);
}

RImage RenderSystem::get_font_atlas_image()
{
    return mObj->get_font_atlas_image();
}

Image2D RenderSystem::create_image_2d(Bitmap bitmap)
{
    if (!bitmap)
        return {};

    RImage image = mObj->create_image_2d(bitmap);
    return Image2D(image.unwrap(), image.get_id());
}
void RenderSystem::destroy_image_2d(Image2D image)
{
    if (!image)
        return;

    mObj->destroy_image_2d(RImage(image.unwrap()));
}

ImageCube RenderSystem::create_image_cube(Bitmap cubemapFaces)
{
    if (!cubemapFaces)
        return {};

    RImage image = mObj->create_image_cube(cubemapFaces);
    return ImageCube(image.unwrap(), image.get_id());
}

void RenderSystem::destroy_image_cube(ImageCube image)
{
    if (!image)
        return;

    mObj->destroy_image_cube(RImage(image.unwrap()));
}

RUID RenderSystem::create_screen_layer(const std::string& name)
{
    RUID layerID = mObj->create_screen_layer(name);

    return layerID;
}

void RenderSystem::destroy_screen_layer(RUID layer)
{
    if (!layer)
        return;

    mObj->destroy_screen_layer(layer);
}

Sprite2DDraw RenderSystem::create_sprite_2d_draw(Image2D image2D, RUID layerID, const Rect& rect, uint32_t zDepth)
{
    LD_ASSERT(layerID);

    Sprite2DDrawObj* obj = mObj->create_sprite_2d_draw(RImage(image2D.unwrap()), layerID, rect, zDepth);

    LD_ASSERT(obj);
    return Sprite2DDraw(obj, obj->id);
}

void RenderSystem::destroy_sprite_2d_draw(Sprite2DDraw draw)
{
    if (!draw)
        return;

    mObj->destroy_sprite_2d_draw(draw.unwrap());
}

MeshData RenderSystem::create_mesh_data(ModelBinary& binary)
{
    MeshDataObj* obj = mObj->create_mesh_data(binary);

    return MeshData(obj, obj->id);
}

void RenderSystem::destroy_mesh_data(MeshData data)
{
    if (!data)
        return;

    mObj->destroy_mesh_data(data.unwrap());
}

MeshDraw RenderSystem::create_mesh_draw()
{
    MeshDrawObj* obj = mObj->create_mesh_draw(nullptr);

    LD_ASSERT(obj);
    return MeshDraw(obj, obj->id);
}

MeshDraw RenderSystem::create_mesh_draw(MeshData data)
{
    if (!data)
        return {};

    MeshDrawObj* obj = mObj->create_mesh_draw(data.unwrap());

    LD_ASSERT(obj);
    return MeshDraw(obj, obj->id);
}

void RenderSystem::destroy_mesh_draw(MeshDraw draw)
{
    if (!draw)
        return;

    mObj->destroy_mesh_draw(draw.unwrap());
}

} // namespace LD
