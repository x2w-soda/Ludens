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
#include <Ludens/RenderServer/RenderServer.h>

namespace LD {

static Log sLog("RServer");

struct MeshData
{
    RMesh mesh;                 /// mesh resources
    MeshDataID dataID;          /// mesh identifier
    HashSet<MeshDrawID> drawID; /// draw ids using this mesh
};

struct Sprite2DDraw
{
    RImage sprite;
    Sprite2DDrawID drawID;
};

/// @brief Render server implementation.
class RenderServerObj
{
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

    RImage create_2d_image(Bitmap bitmap);
    void destroy_2d_image(RImage image);

    MeshDataID create_mesh_data_id(ModelBinary& binary);
    MeshDrawID create_mesh_draw_id(MeshDataID dataID);
    void destroy_mesh_draw_id(MeshDrawID drawID);
    void destroy_all_mesh_draw_id();
    void destroy_all_mesh_data_id();
    inline bool mesh_exists(MeshDataID dataID) { return mMeshData.contains(dataID); }

    Sprite2DDataID create_sprite_2d_data_id(Bitmap bitmap);
    Sprite2DDrawID create_sprite_2d_draw_id(Sprite2DDataID dataID);
    void destroy_sprite_2d_draw_id(Sprite2DDrawID drawID);
    void destroy_all_sprite_2d_draw_id();
    void destroy_all_sprite_2d_data_id();
    inline bool sprite_2d_exists(Sprite2DDataID dataID) { return mSpriteData.contains(dataID); }

    CubemapDataID create_cubemap_data_id(Bitmap cubemapFaces);
    void destroy_cubemap_data_id(CubemapDataID dataID);
    void destroy_all_cubemap_data_id();

    inline RUID get_ruid() { return mRUIDCtr.get_id(); }
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
    RenderServerScreenPassCallback mScreenPassCallback = nullptr;
    void* mScreenPassCallbackUser = nullptr;
    Vec2 mSceneExtent;
    Vec2 mScreenExtent;
    Vec4 mClearColor;
    PoolAllocator mSprite2DDrawPA{};
    Vector<Frame> mFrames;
    Vector<RCommandPool> mCmdPools;
    Vector<RCommandList> mCmdLists;
    HashMap<Sprite2DDataID, RImage> mSpriteData;
    HashMap<CubemapDataID, RImage> mCubemapData;
    HashMap<MeshDataID, MeshData*> mMeshData;             // TODO: optimize later
    HashMap<MeshDrawID, MeshDataID> mMeshDraw;            /// Mesh draw info
    HashMap<Sprite2DDrawID, Sprite2DDraw*> mSprite2DDraw; /// Spirte2D draw info
    RFormat mDepthStencilFormat;                          /// default depth stencil format
    RFormat mColorFormat;                                 /// default color format
    RSampleCountBit mMSAA;                                /// number of samples during MSAA, if enabled
    RUID mSceneOutlineSubject;                            /// subject to be outlined in scene render pass
    uint32_t mFramesInFlight = 0;                         /// number of frames in flight
    uint32_t mFrameIndex = 0;                             /// [0, mFramesInFlight)
    FontAtlas mFontAtlas{};                               /// default font atlas for text rendering
    RGraphImage mLastColorAttachment{};                   /// last color attachment output
    RGraphImage mLastIDFlagsAttachment{};                 /// last scene ID flags attachment output
    bool mHasAcquiredRootWindowImage = false;
    bool mHasAcquiredDialogWindowImage = false;
};

RenderServerObj::RenderServerObj(const RenderServerInfo& serverI)
    : mDevice(serverI.device), mColorFormat(RFORMAT_RGBA8), mFontAtlas(serverI.fontAtlas)
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

    PoolAllocatorInfo paI{};
    paI.blockSize = sizeof(Sprite2DDraw);
    paI.pageSize = 256;
    paI.usage = MEMORY_USAGE_RENDER;
    paI.isMultiPage = true;
    mSprite2DDrawPA = PoolAllocator::create(paI);

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
    LD_PROFILE_SCOPE;

    mDevice.wait_idle();

    RGraph::release(mDevice);

    destroy_all_mesh_draw_id();
    destroy_all_mesh_data_id();
    destroy_all_sprite_2d_draw_id();
    destroy_all_sprite_2d_data_id();
    destroy_all_cubemap_data_id();

    PoolAllocator::destroy(mSprite2DDrawPA);

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

    if (mCubemapData.contains(frameI.envCubemap))
    {
        RImage envCubemap = mCubemapData[frameI.envCubemap];
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

void RenderServerObj::submit_frame()
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

void RenderServerObj::scene_pass(const RenderServerScenePass& sceneP)
{
    LD_PROFILE_SCOPE;

    if (!mHasAcquiredRootWindowImage)
        return;

    RClearDepthStencilValue clearDS = {.depth = 1.0f, .stencil = 0};

    mSceneOutlineSubject = sceneP.overlay.enabled ? sceneP.overlay.outlineRUID : 0;
    mTransformCallback = sceneP.transformCallback;
    mTransformCallbackUser = sceneP.user;

    ForwardRenderComponentInfo forwardI{};
    forwardI.width = (uint32_t)mSceneExtent.x;
    forwardI.height = (uint32_t)mSceneExtent.y;
    forwardI.colorFormat = mColorFormat;
    forwardI.clearColor = RUtil::make_clear_color(mClearColor.r, mClearColor.g, mClearColor.b, mClearColor.a);
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
}

void RenderServerObj::screen_pass(const RenderServerScreenPass& screenP)
{
    LD_PROFILE_SCOPE;

    if (!mHasAcquiredRootWindowImage)
        return;

    // TODO: mScreenPassLayerCallback = screenP.layerCallback;
    mScreenPassCallback = screenP.callback;
    mScreenPassCallbackUser = screenP.user;

    ScreenRenderComponentInfo screenRCI{};
    screenRCI.format = mColorFormat;
    screenRCI.onDrawCallback = &RenderServerObj::screen_rendering;
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

void RenderServerObj::editor_pass(const RenderServerEditorPass& editorP)
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

void RenderServerObj::editor_overlay_pass(const RenderServerEditorOverlayPass& editorOP)
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

void RenderServerObj::editor_dialog_pass(const RenderServerEditorDialogPass& editorDP)
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

RImage RenderServerObj::create_2d_image(Bitmap bitmap)
{
    LD_PROFILE_SCOPE;

    RImageInfo imageI = RUtil::make_2d_image_info(RIMAGE_USAGE_SAMPLED_BIT | RIMAGE_USAGE_TRANSFER_DST_BIT, RFORMAT_RGBA8, bitmap.width(), bitmap.height());
    imageI.sampler = {RFILTER_LINEAR, RFILTER_LINEAR, RSAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE};
    RImage image = mDevice.create_image(imageI);

    RStager stager(mDevice, RQUEUE_TYPE_GRAPHICS);
    stager.add_image_data(image, bitmap.data(), RIMAGE_LAYOUT_SHADER_READ_ONLY);
    stager.submit(mDevice.get_graphics_queue());

    return image;
}

void RenderServerObj::destroy_2d_image(RImage image)
{
    LD_PROFILE_SCOPE;

    mDevice.wait_idle();

    mDevice.destroy_image(image);
}

void RenderServerObj::destroy_all_mesh_draw_id()
{
    LD_PROFILE_SCOPE;

    for (auto it : mMeshData)
    {
        MeshData* data = it.second;

        for (MeshDrawID drawID : data->drawID)
            mMeshDraw.erase(drawID);

        data->drawID.clear();
    }
}

void RenderServerObj::destroy_all_mesh_data_id()
{
    LD_PROFILE_SCOPE;

    mDevice.wait_idle();

    // all draws are out of date.
    destroy_all_mesh_draw_id();

    for (auto ite : mMeshData)
    {
        MeshData* data = ite.second;
        data->mesh.destroy();
        heap_delete<MeshData>(data);
    }

    mMeshData.clear();
}

MeshDataID RenderServerObj::create_mesh_data_id(ModelBinary& binary)
{
    RStager stager(mDevice, RQUEUE_TYPE_GRAPHICS);

    MeshDataID dataID = get_ruid();
    MeshData* entry = heap_new<MeshData>(MEMORY_USAGE_RENDER);
    mMeshData[dataID] = entry;

    entry->mesh.create_from_binary(mDevice, stager, binary);
    entry->dataID = dataID;
    stager.submit(mDevice.get_graphics_queue());

    return dataID;
}

MeshDrawID RenderServerObj::create_mesh_draw_id(MeshDataID dataID)
{
    auto ite = mMeshData.find(dataID);

    if (ite == mMeshData.end())
        return 0;

    MeshData* data = mMeshData[dataID];
    MeshDrawID drawID = get_ruid();
    data->drawID.insert(drawID);
    mMeshDraw[drawID] = dataID;

    return drawID;
}

void RenderServerObj::destroy_mesh_draw_id(MeshDrawID drawID)
{
    auto ite = mMeshDraw.find(drawID);

    if (ite == mMeshDraw.end())
        return;

    RUID meshID = mMeshDraw[drawID];
    mMeshDraw.erase(drawID);

    MeshData* data = mMeshData[meshID];

    data->drawID.erase(drawID);
}

Sprite2DDataID RenderServerObj::create_sprite_2d_data_id(Bitmap bitmap)
{
    RImage sprite = create_2d_image(bitmap);
    Sprite2DDataID dataID = get_ruid();
    mSpriteData[dataID] = sprite;

    return dataID;
}

Sprite2DDrawID RenderServerObj::create_sprite_2d_draw_id(Sprite2DDataID dataID)
{
    auto ite = mSpriteData.find(dataID);

    if (ite == mSpriteData.end())
        return 0;

    Sprite2DDraw* draw = (Sprite2DDraw*)mSprite2DDrawPA.allocate();
    draw->sprite = ite->second;
    draw->drawID = get_ruid();

    mSprite2DDraw[draw->drawID] = draw;

    return draw->drawID;
}

void RenderServerObj::destroy_sprite_2d_draw_id(Sprite2DDrawID drawID)
{
    auto it = mSprite2DDraw.find(drawID);

    if (it == mSprite2DDraw.end())
        return;

    Sprite2DDraw* draw = it->second;

    mSprite2DDraw.erase(it);
    mSprite2DDrawPA.free(draw);
}

void RenderServerObj::destroy_all_sprite_2d_draw_id()
{
    LD_PROFILE_SCOPE;

    for (auto it : mSprite2DDraw)
        mSprite2DDrawPA.free(it.second);

    mSprite2DDraw.clear();
}

void RenderServerObj::destroy_all_sprite_2d_data_id()
{
    LD_PROFILE_SCOPE;

    mDevice.wait_idle();

    // all draws are out of date.
    destroy_all_sprite_2d_draw_id();

    for (auto ite : mSpriteData)
    {
        RImage sprite = ite.second;
        mDevice.destroy_image(sprite);
    }

    mSpriteData.clear();
}

CubemapDataID RenderServerObj::create_cubemap_data_id(Bitmap cubemapFaces)
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

    CubemapDataID dataID = get_ruid();
    mCubemapData[dataID] = cubemap;

    return dataID;
}

void RenderServerObj::destroy_cubemap_data_id(CubemapDataID dataID)
{
    auto ite = mCubemapData.find(dataID);

    if (ite == mCubemapData.end())
        return;

    RImage cubemap = ite->second;
    mCubemapData.erase(ite);

    mDevice.wait_idle();
    mDevice.destroy_image(cubemap);
}

void RenderServerObj::destroy_all_cubemap_data_id()
{
    LD_PROFILE_SCOPE;

    for (auto ite : mCubemapData)
    {
        RImage cubemap = ite.second;
        mDevice.destroy_image(cubemap);
    }

    mCubemapData.clear();
}

// NOTE: This is super early placeholder scene renderer implementation.
//       Once other engine subsystems such as Assets and Scenes are resolved,
//       we will come back and replace this silly procedure.
void RenderServerObj::forward_rendering(ForwardRenderComponent renderer, void* user)
{
    LD_PROFILE_SCOPE;

    RenderServerObj& self = *(RenderServerObj*)user;
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
        MeshData* data = ite.second;

        for (MeshDrawID drawID : data->drawID)
        {
            pc.model = self.mTransformCallback(drawID, self.mTransformCallbackUser);
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
        MeshDataID dataID = self.mMeshDraw[outlineDrawID];
        LD_ASSERT(self.mMeshData.contains(dataID));
        MeshData* data = self.mMeshData[dataID];

        // render to 16-bit flags only
        meshPipeline.set_color_write_mask(0, 0);
        meshPipeline.set_color_write_mask(1, RCOLOR_COMPONENT_B_BIT | RCOLOR_COMPONENT_A_BIT);
        meshPipeline.set_depth_test_enable(false);

        pc.model = self.mTransformCallback(outlineDrawID, self.mTransformCallbackUser);
        pc.id = 0;    // not written to color attachment due to write masks
        pc.flags = 1; // currently any non-zero flag value indicates mesh that requires outlining

        renderer.set_push_constant(sRMeshPipelineLayout, 0, sizeof(pc), &pc);
        renderer.draw_mesh(data->mesh);
    }

    renderer.draw_skybox();
}

void RenderServerObj::screen_rendering(ScreenRenderComponent renderer, void* user)
{
    LD_PROFILE_SCOPE;

    RenderServerObj& self = *(RenderServerObj*)user;

    if (!self.mHasAcquiredRootWindowImage)
        return;

    /*
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
    */

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

RImage RenderServer::create_image(Bitmap bitmap)
{
    LD_ASSERT(bitmap);

    return mObj->create_2d_image(bitmap);
}

void RenderServer::destroy_image(RImage image)
{
    LD_ASSERT(image);

    mObj->destroy_2d_image(image);
}

RImage RenderServer::get_font_atlas_image()
{
    return mObj->get_font_atlas_image();
}

//
// Sprite2D
//

bool RenderServer::ISprite2D::exists(Sprite2DDataID dataID)
{
    LD_ASSERT(dataID);

    return mObj->sprite_2d_exists(dataID);
}

Sprite2DDataID RenderServer::ISprite2D::create_data_id(Bitmap bitmap)
{
    LD_ASSERT(bitmap && bitmap.format() == BITMAP_FORMAT_RGBA8U);

    return mObj->create_sprite_2d_data_id(bitmap);
}

Sprite2DDataID RenderServer::ISprite2D::create_draw_id(Sprite2DDataID dataID)
{
    LD_ASSERT(dataID);

    return mObj->create_sprite_2d_draw_id(dataID);
}

void RenderServer::ISprite2D::destroy_draw_id(Sprite2DDrawID drawID)
{
    LD_ASSERT(drawID);

    mObj->destroy_sprite_2d_draw_id(drawID);
}

void RenderServer::ISprite2D::destroy_all_draw_id()
{
    mObj->destroy_all_sprite_2d_draw_id();
}

//
// Mesh
//

bool RenderServer::IMesh::exists(MeshDataID dataID)
{
    LD_ASSERT(dataID);

    return mObj->mesh_exists(dataID);
}

MeshDataID RenderServer::IMesh::create_data_id(ModelBinary& binary)
{
    return mObj->create_mesh_data_id(binary);
}

MeshDrawID RenderServer::IMesh::create_draw_id(MeshDataID dataID)
{
    LD_ASSERT(dataID);

    return mObj->create_mesh_draw_id(dataID);
}

void RenderServer::IMesh::destroy_draw_id(MeshDrawID drawID)
{
    LD_ASSERT(drawID);

    mObj->destroy_mesh_draw_id(drawID);
}

void RenderServer::IMesh::destroy_all_data_id()
{
    mObj->destroy_all_mesh_data_id();
}

void RenderServer::IMesh::destroy_all_draw_id()
{
    mObj->destroy_all_mesh_draw_id();
}

//
// Cubemap
//

CubemapDataID RenderServer::ICubemap::create_data_id(Bitmap cubemapFaces)
{
    LD_ASSERT(cubemapFaces);

    return mObj->create_cubemap_data_id(cubemapFaces);
}

void RenderServer::ICubemap::destroy_data_id(CubemapDataID dataID)
{
    LD_ASSERT(dataID);

    mObj->destroy_cubemap_data_id(dataID);
}

} // namespace LD
