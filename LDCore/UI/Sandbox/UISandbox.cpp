#include "UISandbox.h"
#include <LDUtil/LudensLFS/Include/LudensLFS.h>
#include <Ludens/Application/Application.h>
#include <Ludens/Application/Input.h>
#include <Ludens/JobSystem/JobSystem.h>
#include <Ludens/Log/Log.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/RenderBackend/RStager.h>
#include <Ludens/RenderBackend/RUtil.h>
#include <Ludens/System/FileSystem.h>
#include <Ludens/UI/UIImmediate.h>
#include <LudensEditor/EditorContext/EditorIconAtlas.h>
#include <array>
#include <string>
#include <vector>

namespace LD {

static Log sLog("UISandbox");

UISandbox::UISandbox()
{
    LD_PROFILE_SCOPE;

    sLog.info("pwd: {}", std::filesystem::current_path().string());

    JobSystemInfo jsI{};
    jsI.immediateQueueCapacity = 128;
    jsI.standardQueueCapacity = 128;
    JobSystem::init(jsI);

    ApplicationInfo appI{};
    appI.width = 1600;
    appI.height = 900;
    appI.name = "UISandbox";
    appI.onEvent = &UISandbox::on_event;
    appI.user = this;
    appI.hintBorderColor = 0;
    appI.hintTitleBarColor = 0x000000FF;
    appI.hintTitleBarTextColor = 0xDFDFDFFF;
    Application app = Application::create(appI);
    Vec2 screenExtent((float)app.width(), (float)app.height());

    CameraPerspectiveInfo perspectiveI{};
    perspectiveI.aspectRatio = (float)app.width() / (float)app.height();
    perspectiveI.nearClip = 0.1f;
    perspectiveI.farClip = 100.0f;
    perspectiveI.fov = LD_TO_RADIANS(45.0f);
    mCamera = Camera::create(perspectiveI, Vec3(0.0f));

    std::string fontPathString = sLudensLFS.fontPath.string();
    mFont = Font::create_from_path(fontPathString.c_str());
    mFontAtlas = FontAtlas::create_bitmap(mFont, 32.0f);

    RDeviceInfo deviceI{};
    deviceI.backend = RDEVICE_BACKEND_VULKAN;
    deviceI.window = app.get_glfw_window();
    deviceI.vsync = true;
    mRDevice = RDevice::create(deviceI);

    RServerInfo serverI{};
    serverI.device = mRDevice;
    serverI.fontAtlas = mFontAtlas;
    mRServer = RServer::create(serverI);

    mFontAtlasImage = mRServer.get_font_atlas_image();

    {
        Bitmap tmpBitmap = Bitmap::create_from_path(sLudensLFS.materialIconsPath.string().c_str(), false);
        RImageInfo imageI = RUtil::make_2d_image_info(RIMAGE_USAGE_SAMPLED_BIT | RIMAGE_USAGE_TRANSFER_DST_BIT,
                                                      RFORMAT_RGBA8, tmpBitmap.width(), tmpBitmap.height(),
                                                      {.filter = RFILTER_LINEAR, .mipmapFilter = RFILTER_LINEAR, .addressMode = RSAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE});

        mIconAtlasImage = mRDevice.create_image(imageI);

        RStager stager(mRDevice, RQUEUE_TYPE_GRAPHICS);
        stager.add_image_data(mIconAtlasImage, tmpBitmap.data(), RIMAGE_LAYOUT_SHADER_READ_ONLY);
        stager.submit(mRDevice.get_graphics_queue());
        Bitmap::destroy(tmpBitmap);
    }

    static UIThemeInfo sUITheme = UITheme::get_default_info();
    UIWindowManagerInfo wmI{};
    wmI.topBarHeight = 20.0f;
    wmI.bottomBarHeight = 20.0f;
    wmI.theme = UITheme(&sUITheme);
    wmI.screenSize = screenExtent;
    wmI.fontAtlas = mFontAtlas;
    wmI.fontAtlasImage = mFontAtlasImage;
    wmI.iconAtlasImage = mIconAtlasImage;
    wmI.icons.close = EditorIconAtlas::get_icon_rect(EditorIcon::Close);
    mUIWM = UIWindowManager::create(wmI);

    // create floating client window
    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_Y;
    layoutI.sizeX = UISize::fixed(250.0f);
    layoutI.sizeY = UISize::fixed(400.0f);
    UIWindowInfo windowI{};
    windowI.name = "demo";
    windowI.defaultMouseControls = false;
    windowI.drawWithScissor = false;
    mClient = mUIWM.get_context().add_window(layoutI, windowI, nullptr);
    mClient.layout();
    mClient.set_on_draw([](UIWidget widget, ScreenRenderComponent renderer) {
        renderer.draw_rect(widget.get_rect(), Color(0x303030FF));
    });
    UIWMClientInfo clientI{};
    clientI.client = mClient;
    clientI.user = nullptr;
    UIWMAreaID areaID = mUIWM.create_float(clientI);
    mUIWM.show_float(areaID);
}

UISandbox::~UISandbox()
{
    LD_PROFILE_SCOPE;

    mRDevice.wait_idle();
    mRDevice.destroy_image(mIconAtlasImage);
    mUIWM.get_context().remove_window(mClient);

    UIWindowManager::destroy(mUIWM);
    Camera::destroy(mCamera);
    RServer::destroy(mRServer);
    RDevice::destroy(mRDevice);
    FontAtlas::destroy(mFontAtlas);
    Font::destroy(mFont);

    JobSystem::shutdown();
}

void UISandbox::run()
{
    Application app = Application::get();

    while (app.is_window_open())
    {
        app.poll_events();

        if (app.is_window_minimized())
            continue;

        imgui();

        // update and render
        float delta = (float)app.get_delta_time();
        mUIWM.update(delta);
        render();

        LD_PROFILE_FRAME_MARK;
    }

    ui_imgui_release(mUIWM.get_context());

    Application::destroy();
}

void UISandbox::imgui()
{
    ui_frame_begin(mUIWM.get_context());

    bool isPressed;

    ui_push_window("Demo");
    ui_push_scroll();
    {
        if (Input::get_key(KEY_CODE_SPACE))
        {
            ui_push_button("Button1", isPressed);
            if (isPressed)
            {
                sLog.info("Button1 pressed!");
            }
            ui_pop();

            ui_push_button("Button2", isPressed);
            if (isPressed)
            {
                sLog.info("Button2 pressed!");
            }
            ui_pop();
        }

        ui_push_panel();
        {
            ui_push_text("Some text1 here!!!!!!");
            ui_pop();
            ui_push_image(mFontAtlasImage, 300, 300);
            ui_pop();
            ui_push_text("Some text2 here!");
            ui_pop();
        }
        ui_pop();

        static float sValue = 2.0f;
        ui_push_slider(0.0f, 3.0f, &sValue);
        ui_pop();

        ui_push_button("Button3", isPressed);
        if (isPressed)
        {
            sLog.info("Button3 pressed!");
        }
        ui_pop();
    }
    ui_pop();
    ui_pop_window();

    ui_frame_end();
}

void UISandbox::render()
{
    Application app = Application::get();
    Vec2 screenExtent((float)app.width(), (float)app.height());

    // begin rendering a frame
    RServerFrameInfo frameI{};
    frameI.directionalLight = Vec3(0.0f, 1.0f, 0.0f);
    frameI.mainCamera = mCamera;
    frameI.screenExtent = screenExtent;
    frameI.sceneExtent = screenExtent;
    frameI.envCubemap = (RUID)0;
    mRServer.next_frame(frameI);

    // render empty scene
    RServerScenePass sceneP{};
    sceneP.transformCallback = nullptr;
    sceneP.overlay.enabled = false;
    sceneP.hasSkybox = false;
    sceneP.user = this;
    mRServer.scene_pass(sceneP);

    // render UI in screen space
    RServerSceneScreenPass screenP{};
    screenP.renderCallback = &UISandbox::on_screen_render;
    screenP.user = this;
    mRServer.scene_screen_pass(screenP);

    mRServer.submit_frame();
}

void UISandbox::on_event(const Event* event, void* user)
{
    UISandbox& self = *(UISandbox*)user;

    // pass events to UI
    UIContext uiCtx = self.mUIWM.get_context();
    uiCtx.forward_event(event);
}

void UISandbox::on_screen_render(ScreenRenderComponent renderer, void* user)
{
    UISandbox& self = *(UISandbox*)user;

    self.mUIWM.render_float(renderer);
}

} // namespace LD