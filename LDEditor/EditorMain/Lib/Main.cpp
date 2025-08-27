#include "EditorUI.h"
#include <Ludens/Application/Application.h>
#include <Ludens/Application/Event.h>
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

namespace fs = std::filesystem;

namespace LD {

static Log sLog("LDEditor");

class EditorApplication
{
public:
    EditorApplication()
    {
        sLog.info("pwd: {}", fs::current_path().string());

        JobSystemInfo jsI{};
        jsI.immediateQueueCapacity = 128;
        jsI.standardQueueCapacity = 128;
        JobSystem::init(jsI);

        ApplicationInfo appI{};
        appI.width = 1600;
        appI.height = 900;
        appI.vsync = true;
        appI.name = "LDEditor";
        appI.onEvent = &EditorUI::on_event;
        appI.user = &mEditorUI;
        appI.hintBorderColor = 0;
        appI.hintTitleBarColor = 0x000000FF;
        appI.hintTitleBarTextColor = 0xDFDFDFFF;
        Application app = Application::create(appI);

        // TODO: embed font
        mFont = Font::create_from_path("../../../../Assets/ttf/Inter_24pt-Regular.ttf");
        mFontAtlas = FontAtlas::create_sdf(mFont, 24.0f);

        // TODO: remove hardcoded skybox
        fs::path dirPath = "../../../../Assets/skybox/opengl";
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

        RServerInfo serverI{};
        serverI.device = app.get_rdevice();
        serverI.fontAtlas = mFontAtlas;
        serverI.cubemapFaces = tmpCubemapFaces;
        mRServer = RServer::create(serverI);

        Bitmap::destroy(tmpCubemapFaces);

        // load scene into editor context
        EditorContextInfo contextI{};
        contextI.renderServer = mRServer;
        mEditorCtx = EditorContext::create(contextI);
        mEditorCtx.load_project("../../../../Project/project.toml");

        // initalize editor UI
        EditorUIInfo uiI{};
        uiI.ctx = mEditorCtx;
        uiI.fontAtlas = mFontAtlas;
        uiI.fontAtlasImage = mRServer.get_font_atlas_image();
        uiI.screenWidth = appI.width;
        uiI.screenHeight = appI.height;
        mEditorUI.startup(uiI);
    }

    ~EditorApplication()
    {
        mEditorUI.cleanup();

        EditorContext::destroy(mEditorCtx);
        RServer::destroy(mRServer);
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
            mEditorUI.update(delta);

            // If the Scene is playing in editor, this drives the scene update as well
            mEditorCtx.update(delta);

            // begin rendering a frame
            RServerFrameInfo frameI{};
            frameI.directionalLight = Vec3(0.0f, 1.0f, 0.0f);
            frameI.mainCamera = mEditorUI.get_viewport_camera();
            frameI.screenExtent = Vec2((float)app.width(), (float)app.height());
            frameI.sceneExtent = mEditorUI.get_viewport_size();
            mRServer.next_frame(frameI);

            // render game scene with overlay, the editor context is responsible for supplying object transforms
            RServerScenePass sceneP{};
            sceneP.transformCallback = &EditorContext::render_server_transform_callback;
            sceneP.user = mEditorCtx.unwrap();
            sceneP.overlay.enabled = !mEditorCtx.is_playing();
            sceneP.overlay.outlineRUID = mEditorUI.get_viewport_outline_ruid();
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
    RServer mRServer;
    EditorContext mEditorCtx;
    EditorUI mEditorUI;
    Font mFont;
    FontAtlas mFontAtlas;
    RImage mFontAtlasImage;
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