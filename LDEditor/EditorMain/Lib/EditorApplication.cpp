#include <Ludens/JobSystem/JobSystem.h>
#include <Ludens/Log/Log.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/System/FileSystem.h>
#include <Ludens/System/Win32Initialization.h>
#include <Ludens/WindowRegistry/WindowRegistry.h>
#include <LudensBuilder/ProjectBuilder/ProjectScan.h>

#include "EditorApplication.h"

// NOTE: THIS IS TEMPORARY. We are experimenting with editor icons, fonts, and other files.
//       Eventually such files will be embedded in the editor, currently we are fetching
//       from the LFS submodule at: https://github.com/x2w-soda/LudensLFS.
//       Run `git submodule init && git submodule update` from the root folder to
//       fetch the experimental media files in the submodule.
#include <LudensUtil/LudensLFS/LudensLFS.h>

namespace LD {

static Log sLog("LDEditor");

EditorApplication::EditorApplication(const EditorApplicationInfo& info)
{
    LD_PROFILE_SCOPE;

    sLog.info("pwd: {}", FS::current_path().string());

    JobSystemInfo jsI{};
    jsI.immediateQueueCapacity = 128;
    jsI.standardQueueCapacity = 128;
    JobSystem::init(jsI);

    // Initial project discovery as soon as job system is initialized.
    // Probably want some editor manifest file to cache recent projects.
    Vector<FS::Path> projectSchemaPaths;
    projectSchemaPaths.push_back(sLudensLFS.projectPath);
    if (info.projectSchemaPath)
        projectSchemaPaths.push_back(*info.projectSchemaPath);
    begin_project_scans(projectSchemaPaths);

    WindowInfo windowI{};
    windowI.width = 1600;
    windowI.height = 900;
    windowI.name = "Ludens";
    windowI.onEvent = &EditorUI::on_window_event;
    windowI.user = &mEditorUI;
    windowI.hintBorderColor = 0;
    windowI.hintTitleBarColor = 0x000000FF;
    windowI.hintTitleBarTextColor = 0xDFDFDFFF;
    WindowRegistry reg = WindowRegistry::create(windowI);
    const Vec2 screenExtent = reg.get_window_extent(reg.get_root_id());

    // TODO:
    constexpr float editorFontSize = 32.0f;

    std::string fontPathString = sLudensLFS.fontPath.string();
    mFont = Font::create_from_path(fontPathString.c_str());
    mFontAtlas = FontAtlas::create_bitmap(mFont, editorFontSize);

    fontPathString = sLudensLFS.monospaceFontPath.string();
    mMSFont = Font::create_from_path(fontPathString.c_str());
    mMSFontAtlas = FontAtlas::create_bitmap(mMSFont, editorFontSize);

    RDeviceInfo deviceI{};
    deviceI.backend = RDEVICE_BACKEND_VULKAN;
    deviceI.vsync = info.vsync;
    mRDevice = RDevice::create(deviceI);

    RenderSystemInfo serverI{};
    serverI.device = mRDevice;
    serverI.defaultFontAtlas = mFontAtlas;
    serverI.monoFontAtlas = mMSFontAtlas;
    mRenderSystem = RenderSystem::create(serverI);

    win32_initialize_ole();
    mAudioSystem = AudioSystem::create();

    // barrier to wait for all project scans to complete.
    Vector<ProjectScanResult> projectScanResults;
    wait_project_scans(projectScanResults);

    // load scene into editor context
    EditorContextInfo contextI{};
    contextI.audioSystem = mAudioSystem;
    contextI.renderSystem = mRenderSystem;
    contextI.iconAtlasPath = sLudensLFS.editorIconAtlasPath;
    contextI.defaultFontAtlas = mFontAtlas;
    contextI.defaultFontAtlasImage = mRenderSystem.get_font_atlas_image();
    contextI.monoFontAtlas = mMSFontAtlas;
    contextI.monoFontAtlasImage = mRenderSystem.get_mono_font_atlas_image();
    contextI.projectScanResultCount = projectScanResults.size();
    contextI.projectScanResults = projectScanResults.data();
    mEditorCtx = EditorContext::create(contextI);

    GLFWwindow* rootNativeWindow = reg.get_window_glfw_handle(reg.get_root_id());
    mDropTarget = DropTarget::create(rootNativeWindow, &EditorContext::drop_file_callback, mEditorCtx.unwrap());

    /*
    const FS::Path* path = info.projectSchemaPath;
    if (!path) // open sandbox project in LFS repository
        path = &sLudensLFS.projectPath;

    if (path)
    {
        auto* actionE = (EditorActionOpenProjectEvent*)mEditorCtx.enqueue_event(EDITOR_EVENT_TYPE_ACTION_OPEN_PROJECT);
        actionE->projectSchema = *path;
    }

    mEditorCtx.poll_events();
    */

    // initalize editor UI
    EditorUIInfo uiI{};
    uiI.ctx = mEditorCtx;
    uiI.renderSystem = mRenderSystem;
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

    DropTarget::destroy(mDropTarget);
    EditorContext::destroy(mEditorCtx);
    AudioSystem::destroy(mAudioSystem);
    RenderSystem::destroy(mRenderSystem);
    RDevice::destroy(mRDevice);
    FontAtlas::destroy(mMSFontAtlas);
    FontAtlas::destroy(mFontAtlas);
    Font::destroy(mMSFont);
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
        // 1. Process Window Events.
        reg.poll_events();

        if (reg.is_window_minimized(rootID))
            continue;

        float delta = (float)reg.get_delta_time();
        Vec2 screenExtent = reg.get_window_extent(rootID);

        // 2. Editor UI imgui pass.
        //    May generate editor events.
        Vec2 sceneExtent = mEditorUI.update(delta, screenExtent);

        // 3. Process Editor Events.
        //    Note that the current project or scene could change after this.
        mEditorCtx.poll_events();

        // 4. EditorContext update.
        //    If the Scene is playing in editor, this drives the Scene update as well
        mEditorCtx.update(sceneExtent, delta);

        // 5. Draw list recording and submit GPU work.
        mEditorUI.submit_frame();

        LD_PROFILE_FRAME_MARK;
    }
}

void EditorApplication::begin_project_scans(const Vector<FS::Path>& projectSchemas)
{
    LD_PROFILE_SCOPE;

    mProjectScans.resize(projectSchemas.size());

    for (size_t i = 0; i < projectSchemas.size(); i++)
    {
        mProjectScans[i] = ProjectScanAsync::create();
        mProjectScans[i].begin(projectSchemas[i]);
    }
}

void EditorApplication::wait_project_scans(Vector<ProjectScanResult>& projectScanResults)
{
    LD_PROFILE_SCOPE;

    JobSystem::get().wait_all();

    projectScanResults.resize(mProjectScans.size());

    for (size_t i = 0; i < mProjectScans.size(); i++)
    {
        if (!mProjectScans[i].get_result(projectScanResults[i]))
            projectScanResults[i].isProjectSchemaValid = false;

        ProjectScanAsync::destroy(mProjectScans[i]);
    }

    mProjectScans.clear();
}

} // namespace LD