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
    deviceI.vsync = false; // TODO: config
    mRDevice = RDevice::create(deviceI);

    RenderSystemInfo serverI{};
    serverI.device = mRDevice;
    serverI.fontAtlas = mFontAtlas;
    mRenderSystem = RenderSystem::create(serverI);

    mAudioSystem = AudioSystem::create();

#if 0
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
        mEnvCubemap = mRenderSystem.create_image_cube(tmpCubemapFaces);
        Bitmap::destroy(tmpCubemapFaces);
    }
#else
    mEnvCubemap = {};
#endif

    // load scene into editor context
    EditorContextInfo contextI{};
    contextI.audioSystem = mAudioSystem;
    contextI.renderSystem = mRenderSystem;
    contextI.iconAtlasPath = sLudensLFS.materialIconsPath;
    contextI.fontAtlas = mFontAtlas;
    contextI.fontAtlasImage = mRenderSystem.get_font_atlas_image();
    mEditorCtx = EditorContext::create(contextI);
    mEditorCtx.load_project(sLudensLFS.projectPath);

    // initalize editor UI
    EditorUIInfo uiI{};
    uiI.ctx = mEditorCtx;
    uiI.fontAtlas = mFontAtlas;
    uiI.fontAtlasImage = mRenderSystem.get_font_atlas_image();
    uiI.renderSystem = mRenderSystem;
    // uiI.envCubemap = mEnvCubemap.get_id();
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
    if (mEnvCubemap)
        mRenderSystem.destroy_image_cube(mEnvCubemap);

    EditorContext::destroy(mEditorCtx);
    AudioSystem::destroy(mAudioSystem);
    RenderSystem::destroy(mRenderSystem);
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
        // Flush window events.
        reg.poll_events();

        if (reg.is_window_minimized(rootID))
            continue;

        float delta = (float)reg.get_delta_time();

        // Flushes the editor action queue.
        // The current project or scene could change after this.
        mEditorCtx.poll_actions();

        mEditorUI.update(delta);
        mEditorUI.submit_frame();

        LD_PROFILE_FRAME_MARK;
    }
}

} // namespace LD