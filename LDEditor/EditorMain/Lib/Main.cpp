#include "EditorUI.h"
#include <Ludens/Application/Application.h>
#include <Ludens/Application/Event.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/JobSystem/JobSystem.h>
#include <Ludens/Log/Log.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/RenderComponent/ScreenRenderComponent.h>
#include <Ludens/RenderServer/RServer.h>
#include <Ludens/Scene/Scene.h>
#include <array>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

// NOTE: THIS IS TEMPORARY. We are experimenting with editor icons, fonts, and other files.
//       Eventually such files will be embedded in the editor, currently we are fetching
//       from the LFS submodule at: https://github.com/x2w-soda/LudensLFS.
//       Run `git submodule init && git submodule update` from the root folder to
//       fetch the experimental media files in the submodule.
#include <LDUtil/LudensLFS/Include/LudensLFS.h>

namespace fs = std::filesystem;

namespace LD {

static Log sLog("LDEditor");

class EditorApplication
{
public:
    EditorApplication()
    {
        LD_PROFILE_SCOPE;

        sLog.info("pwd: {}", fs::current_path().string());

        JobSystemInfo jsI{};
        jsI.immediateQueueCapacity = 128;
        jsI.standardQueueCapacity = 128;
        JobSystem::init(jsI);

        ApplicationInfo appI{};
        appI.width = 1600;
        appI.height = 900;
        appI.name = "Ludens";
        appI.onEvent = &EditorUI::on_event;
        appI.user = &mEditorUI;
        appI.hintBorderColor = 0;
        appI.hintTitleBarColor = 0x000000FF;
        appI.hintTitleBarTextColor = 0xDFDFDFFF;
        Application app = Application::create(appI);

        std::string fontPathString = sLudensLFS.fontPath.string();
        mFont = Font::create_from_path(fontPathString.c_str());
        mFontAtlas = FontAtlas::create_bitmap(mFont, 32.0f);

        RDeviceInfo deviceI{};
        deviceI.backend = RDEVICE_BACKEND_VULKAN;
        deviceI.window = app.get_glfw_window();
        deviceI.vsync = true; // TODO: config
        mRDevice = RDevice::create(deviceI);

        RServerInfo serverI{};
        serverI.device = mRDevice;
        serverI.fontAtlas = mFontAtlas;
        mRServer = RServer::create(serverI);

        {
            fs::path dirPath = sLudensLFS.skyboxFolderPath;
            std::array<std::string, 6> facePaths;
            facePaths[0] = fs::path(dirPath).append("px.png").string();
            facePaths[1] = fs::path(dirPath).append("nx.png").string();
            facePaths[2] = fs::path(dirPath).append("py.png").string();
            facePaths[3] = fs::path(dirPath).append("ny.png").string();
            facePaths[4] = fs::path(dirPath).append("pz.png").string();
            facePaths[5] = fs::path(dirPath).append("nz.png").string();
            std::array<const char*, 6> facePathsCstr;
            for (int i = 0; i < 6; i++)
                facePathsCstr[i] = facePaths[i].c_str();

            Bitmap tmpCubemapFaces = Bitmap::create_cubemap_from_paths(facePathsCstr.data());
            mEnvCubemap = mRServer.create_cubemap(tmpCubemapFaces);
            Bitmap::destroy(tmpCubemapFaces);
        }

        // load scene into editor context
        EditorContextInfo contextI{};
        contextI.renderServer = mRServer;
        contextI.iconAtlasPath = sLudensLFS.materialIconsPath;
        mEditorCtx = EditorContext::create(contextI);
        mEditorCtx.load_project(sLudensLFS.projectPath);

        // initalize editor UI
        EditorUIInfo uiI{};
        uiI.ctx = mEditorCtx;
        uiI.fontAtlas = mFontAtlas;
        uiI.fontAtlasImage = mRServer.get_font_atlas_image();
        uiI.screenWidth = appI.width;
        uiI.screenHeight = appI.height;
        uiI.barHeight = 22;
        mEditorUI.startup(uiI);
    }

    ~EditorApplication()
    {
        LD_PROFILE_SCOPE;

        mEditorUI.cleanup();

        mRDevice.wait_idle();
        mRServer.destroy_cubemap(mEnvCubemap);

        EditorContext::destroy(mEditorCtx);
        RServer::destroy(mRServer);
        RDevice::destroy(mRDevice);
        FontAtlas::destroy(mFontAtlas);
        Font::destroy(mFont);
        Application::destroy();
        JobSystem::shutdown();
    }

    void run()
    {
        Application app = Application::get();

        while (app.is_window_open())
        {
            app.poll_events();

            if (app.is_window_minimized())
                continue;

            float delta = (float)app.get_delta_time();

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
            RServerFrameInfo frameI{};
            frameI.directionalLight = Vec3(0.0f, 1.0f, 0.0f);
            frameI.mainCamera = mainCamera;
            frameI.screenExtent = Vec2((float)app.width(), (float)app.height());
            frameI.sceneExtent = mEditorUI.get_viewport_scene_size();
            frameI.envCubemap = mEnvCubemap;
            mRServer.next_frame(frameI);

            // render game scene with overlay, the editor context is responsible for supplying object transforms
            RServerScenePass sceneP{};
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
            mRServer.scene_pass(sceneP);

            // render the editor UI
            RServerEditorPass editorP{};
            editorP.renderCallback = &EditorUI::on_render;
            editorP.scenePickCallback = &EditorUI::on_scene_pick;
            editorP.user = &mEditorUI;
            editorP.sceneMousePickQuery = nullptr;
            Vec2 queryPos;
            if (mEditorUI.get_viewport_mouse_pos(queryPos))
                editorP.sceneMousePickQuery = &queryPos;
            mRServer.editor_pass(editorP);

            // render the editor overlay UI
            RServerEditorOverlayPass editorOP{};
            editorOP.renderCallback = &EditorUI::on_overlay_render;
            editorOP.blurMixColor = 0x101010FF;
            editorOP.blurMixFactor = 0.1f;
            editorOP.user = &mEditorUI;
            mRServer.editor_overlay_pass(editorOP);

            mRServer.submit_frame();

            LD_PROFILE_FRAME_MARK;
        }
    }

private:
    RDevice mRDevice;
    RServer mRServer;
    EditorContext mEditorCtx;
    EditorUI mEditorUI;
    Font mFont;
    FontAtlas mFontAtlas;
    RImage mFontAtlasImage;
    RUID mEnvCubemap;
};

} // namespace LD

int main(int argc, char** argv)
{
    {
        LD::EditorApplication editorApp;
        editorApp.run();
    }

    int count = LD::get_memory_leaks(nullptr);

    if (count > 0)
    {
        std::vector<LD::MemoryProfile> leaks(count);
        LD::get_memory_leaks(leaks.data());

        for (int i = 0; i < count; i++)
            std::cout << "memory leak in usage " << LD::get_memory_usage_cstr(leaks[i].usage) << ": " << leaks[i].current << " bytes" << std::endl;
    }
}