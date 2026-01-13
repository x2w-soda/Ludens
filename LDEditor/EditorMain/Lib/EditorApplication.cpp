#include <Ludens/Log/Log.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/System/FileSystem.h>
#include <Ludens/WindowRegistry/WindowRegistry.h>

#include "EditorApplication.h"

// NOTE: THIS IS TEMPORARY. We are experimenting with editor icons, fonts, and other files.
//       Eventually such files will be embedded in the editor, currently we are fetching
//       from the LFS submodule at: https://github.com/x2w-soda/LudensLFS.
//       Run `git submodule init && git submodule update` from the root folder to
//       fetch the experimental media files in the submodule.
#include <LudensUtil/LudensLFS.h>

namespace LD {

static Log sLog("LDEditor");

EditorApplication::EditorApplication()
{
    LD_PROFILE_SCOPE;

    sLog.info("pwd: {}", FS::current_path().string());

    JobSystemInfo jsI{};
    jsI.immediateQueueCapacity = 128;
    jsI.standardQueueCapacity = 128;
    JobSystem::init(jsI);

    WindowInfo windowI{};
    windowI.width = 1600;
    windowI.height = 900;
    windowI.name = "Ludens";
    windowI.onEvent = &EditorUI::on_event;
    windowI.user = &mEditorUI;
    windowI.hintBorderColor = 0;
    windowI.hintTitleBarColor = 0x000000FF;
    windowI.hintTitleBarTextColor = 0xDFDFDFFF;
    WindowRegistry reg = WindowRegistry::create(windowI);
    const Vec2 screenExtent = reg.get_window_extent(reg.get_root_id());

    std::string fontPathString = sLudensLFS.fontPath.string();
    mFont = Font::create_from_path(fontPathString.c_str());
    mFontAtlas = FontAtlas::create_bitmap(mFont, 32.0f);

    RDeviceInfo deviceI{};
    deviceI.backend = RDEVICE_BACKEND_VULKAN;
    deviceI.vsync = true; // TODO: config
    mRDevice = RDevice::create(deviceI);

    RenderServerInfo serverI{};
    serverI.device = mRDevice;
    serverI.fontAtlas = mFontAtlas;
    mRenderServer = RenderServer::create(serverI);

    mAudioServer = AudioServer::create();

    {
        FS::Path dirPath = sLudensLFS.skyboxFolderPath;
        std::array<std::string, 6> facePaths;
        facePaths[0] = FS::Path(dirPath).append("px.png").string();
        facePaths[1] = FS::Path(dirPath).append("nx.png").string();
        facePaths[2] = FS::Path(dirPath).append("py.png").string();
        facePaths[3] = FS::Path(dirPath).append("ny.png").string();
        facePaths[4] = FS::Path(dirPath).append("pz.png").string();
        facePaths[5] = FS::Path(dirPath).append("nz.png").string();
        std::array<const char*, 6> facePathsCstr;
        for (int i = 0; i < 6; i++)
            facePathsCstr[i] = facePaths[i].c_str();

        Bitmap tmpCubemapFaces = Bitmap::create_cubemap_from_paths(facePathsCstr.data());
        mEnvCubemap = mRenderServer.create_cubemap(tmpCubemapFaces);
        Bitmap::destroy(tmpCubemapFaces);
    }

    // load scene into editor context
    EditorContextInfo contextI{};
    contextI.audioServer = mAudioServer;
    contextI.renderServer = mRenderServer;
    contextI.iconAtlasPath = sLudensLFS.materialIconsPath;
    mEditorCtx = EditorContext::create(contextI);
    mEditorCtx.load_project(sLudensLFS.projectPath);

    // initalize editor UI
    EditorUIInfo uiI{};
    uiI.ctx = mEditorCtx;
    uiI.fontAtlas = mFontAtlas;
    uiI.fontAtlasImage = mRenderServer.get_font_atlas_image();
    uiI.screenWidth = (uint32_t)screenExtent.x;
    uiI.screenHeight = (uint32_t)screenExtent.y;
    uiI.barHeight = 22;
    mEditorUI.startup(uiI);
}

EditorApplication::~EditorApplication()
{
    LD_PROFILE_SCOPE;

    mEditorUI.cleanup();

    mRDevice.wait_idle();
    mRenderServer.destroy_cubemap(mEnvCubemap);

    EditorContext::destroy(mEditorCtx);
    AudioServer::destroy(mAudioServer);
    RenderServer::destroy(mRenderServer);
    RDevice::destroy(mRDevice);
    FontAtlas::destroy(mFontAtlas);
    Font::destroy(mFont);
    WindowRegistry::destroy();
    JobSystem::shutdown();
}

void EditorApplication::run()
{
    WindowRegistry reg = WindowRegistry::get();
    WindowID rootID = reg.get_root_id();

    while (reg.is_window_open(rootID))
    {
        reg.poll_events();

        if (reg.is_window_minimized(rootID))
            continue;

        float delta = (float)reg.get_delta_time();
        const Vec2 screenExtent = reg.get_window_extent(rootID);

        // The current project or scene could change after this.
        mEditorCtx.poll_actions();

        mEditorUI.update(delta);

        // If the Scene is playing in editor, this drives the scene update as well
        mEditorCtx.update(mEditorUI.get_viewport_scene_size(), delta);

        // If the Scene is playing, the main camera is from some camera component registered in scene.
        // Otherwise it's just the viewport camera.
        Camera mainCamera = mEditorUI.get_main_camera();
        LD_ASSERT(mainCamera);

        // begin rendering a frame
        RenderServerFrameInfo frameI{};
        frameI.directionalLight = Vec3(0.0f, 1.0f, 0.0f);
        frameI.mainCamera = mainCamera;
        frameI.screenExtent = screenExtent;
        frameI.sceneExtent = mEditorUI.get_viewport_scene_size();
        frameI.envCubemap = mEnvCubemap;
        mRenderServer.next_frame(frameI);

        // render game scene with overlay, the editor context is responsible for supplying object transforms
        RenderServerScenePass sceneP{};
        sceneP.transformCallback = &EditorContext::render_server_transform_callback;
        sceneP.user = mEditorCtx.unwrap();
        sceneP.overlay.enabled = !mEditorCtx.is_playing();
        sceneP.overlay.outlineRUID = mEditorUI.get_viewport_outline_ruid();
        sceneP.hasSkybox = (mEnvCubemap != 0);
        mEditorUI.get_viewport_gizmo_state(
            sceneP.overlay.gizmoType,
            sceneP.overlay.gizmoCenter,
            sceneP.overlay.gizmoScale,
            sceneP.overlay.gizmoColor);
        mRenderServer.scene_pass(sceneP);

        // render screen space items on top of game scene.
        RenderServerScreenPass screenP{};
        screenP.layerCallback = &EditorContext::render_server_screen_pass_callback;
        screenP.user = mEditorCtx.unwrap();
        mRenderServer.screen_pass(screenP);

        // render the editor UI
        RenderServerEditorPass editorP{};
        editorP.renderCallback = &EditorUI::on_render;
        editorP.scenePickCallback = &EditorUI::on_scene_pick;
        editorP.user = &mEditorUI;
        editorP.sceneMousePickQuery = nullptr;
        Vec2 queryPos;
        if (mEditorUI.get_viewport_mouse_pos(queryPos))
            editorP.sceneMousePickQuery = &queryPos;
        mRenderServer.editor_pass(editorP);

        // render the editor overlay UI
        RenderServerEditorOverlayPass editorOP{};
        editorOP.renderCallback = &EditorUI::on_render_overlay;
        editorOP.blurMixColor = 0x101010FF;
        editorOP.blurMixFactor = 0.1f;
        editorOP.user = &mEditorUI;
        mRenderServer.editor_overlay_pass(editorOP);

        mRenderServer.submit_frame();

        LD_PROFILE_FRAME_MARK;
    }
}

} // namespace LD