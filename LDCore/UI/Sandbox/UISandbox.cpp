#include <Ludens/DSA/Vector.h>
#include <Ludens/Event/WindowEvent.h>
#include <Ludens/JobSystem/JobSystem.h>
#include <Ludens/Log/Log.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/RenderBackend/RStager.h>
#include <Ludens/RenderBackend/RUtil.h>
#include <Ludens/System/FileSystem.h>
#include <Ludens/UI/UIImmediate.h>
#include <Ludens/WindowRegistry/Input.h>
#include <Ludens/WindowRegistry/WindowRegistry.h>
#include <LudensEditor/EditorContext/EditorIconAtlas.h>
#include <LudensUtil/LudensLFS.h>

#include <array>
#include <string>

#include "UISandbox.h"

namespace LD {

static Log sLog("UISandbox");

static WindowID sWindowID;

UISandbox::UISandbox()
{
    LD_PROFILE_SCOPE;

    sLog.info("pwd: {}", std::filesystem::current_path().string());

    JobSystemInfo jsI{};
    jsI.immediateQueueCapacity = 128;
    jsI.standardQueueCapacity = 128;
    JobSystem::init(jsI);

    WindowInfo windowI{};
    windowI.width = 1600;
    windowI.height = 900;
    windowI.name = "UISandbox";
    windowI.onEvent = &UISandbox::on_event;
    windowI.user = this;
    windowI.hintBorderColor = 0;
    windowI.hintTitleBarColor = 0x000000FF;
    windowI.hintTitleBarTextColor = 0xDFDFDFFF;
    WindowRegistry reg = WindowRegistry::create(windowI);
    const WindowID rootID = reg.get_root_id();
    const Vec2 screenExtent = reg.get_window_extent(rootID);

    CameraPerspectiveInfo perspectiveI{};
    perspectiveI.aspectRatio = reg.get_window_aspect_ratio(rootID);
    perspectiveI.nearClip = 0.1f;
    perspectiveI.farClip = 100.0f;
    perspectiveI.fov = LD_TO_RADIANS(45.0f);
    mCamera = Camera::create(perspectiveI, Vec3(0.0f));

    std::string fontPathString = sLudensLFS.fontPath.string();
    mFont = Font::create_from_path(fontPathString.c_str());
    mFontAtlas = FontAtlas::create_bitmap(mFont, 32.0f);

    RDeviceInfo deviceI{};
    deviceI.backend = RDEVICE_BACKEND_VULKAN;
    deviceI.vsync = true;
    mRDevice = RDevice::create(deviceI);

    windowI.width = 512;
    windowI.height = 512;
    windowI.name = "secondary";
    sWindowID = reg.create_window(windowI, rootID);

    RenderSystemInfo serverI{};
    serverI.device = mRDevice;
    serverI.fontAtlas = mFontAtlas;
    mRenderSystem = RenderSystem::create(serverI);

    mFontAtlasImage = mRenderSystem.get_font_atlas_image();

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

    UIContextInfo ctxI{};
    ctxI.fontAtlas = mFontAtlas;
    ctxI.fontAtlasImage = mFontAtlasImage;
    ctxI.theme = UITheme(&sUITheme);
    mCtx = UIContext::create(ctxI);
    UILayer groundLayer = mCtx.create_layer("ground");
    UILayer floatLayer = mCtx.create_layer("float");
    UIWorkspace space = floatLayer.create_workspace(Rect(100.0f, 50.0f, 250.0f, 400.0f));

    // create floating client window
    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_Y;
    UIWindowInfo uiWindowI{};
    uiWindowI.name = "demo";
    uiWindowI.defaultMouseControls = false;
    uiWindowI.drawWithScissor = false;
    mDemo = space.create_window(space.get_root_id(), layoutI, uiWindowI, nullptr);
    mDemo.layout();
    mDemo.set_on_draw([](UIWidget widget, ScreenRenderComponent renderer) {
        renderer.draw_rect(widget.get_rect(), Color(0x303030FF));
    });
}

UISandbox::~UISandbox()
{
    LD_PROFILE_SCOPE;

    mRDevice.wait_idle();
    mRDevice.destroy_image(mIconAtlasImage);

    UIContext::destroy(mCtx);
    Camera::destroy(mCamera);
    RenderSystem::destroy(mRenderSystem);
    RDevice::destroy(mRDevice);
    FontAtlas::destroy(mFontAtlas);
    Font::destroy(mFont);
    WindowRegistry::destroy();

    JobSystem::shutdown();
}

void UISandbox::run()
{
    WindowRegistry reg = WindowRegistry::get();
    WindowID rootID = reg.get_root_id();

    while (reg.is_window_open(rootID))
    {
        reg.poll_events();

        if (reg.is_window_minimized(rootID))
            continue;

        imgui();

        // update and render
        float delta = (float)reg.get_delta_time();
        mCtx.update(delta);
        render();

        LD_PROFILE_FRAME_MARK;
    }

    ui_imgui_release(mCtx);
}

void UISandbox::imgui()
{
    ui_frame_begin(mCtx);

    bool isPressed;

    ui_push_window(mDemo);
    ui_push_scroll({});
    ui_top_layout_child_gap(10.0f);
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

        std::string text;
        ui_push_text_edit();
        if (ui_text_edit_changed(text))
            sLog.info("Text Changed: [{}]", text);
        if (ui_text_edit_submitted(text))
            sLog.info("Text Submitted: [{}]", text);
        ui_pop();
    }
    ui_pop();
    ui_pop_window();

    ui_frame_end();
}

void UISandbox::render()
{
    WindowRegistry reg = WindowRegistry::get();
    const Vec2 screenExtent = reg.get_window_extent(reg.get_root_id());
    const bool hasDialogWindow = reg.is_window_open(sWindowID);

    // begin rendering a frame
    RenderSystemFrameInfo frameI{};
    frameI.directionalLight = Vec3(0.0f, 1.0f, 0.0f);
    frameI.mainCamera = mCamera;
    frameI.screenExtent = screenExtent;
    frameI.sceneExtent = screenExtent;
    frameI.envCubemap = (RUID)0;
    frameI.dialogWindowID = hasDialogWindow ? sWindowID : 0;
    mRenderSystem.next_frame(frameI);

    if (hasDialogWindow)
    {
        RenderSystemEditorDialogPass editorDP{};
        editorDP.dialogWindow = sWindowID;
        editorDP.renderCallback = &UISandbox::on_dialog_render;
        editorDP.user = this;
        mRenderSystem.editor_dialog_pass(editorDP);
    }

    // render empty scene
    RenderSystemScenePass sceneP{};
    sceneP.mat4Callback = nullptr;
    sceneP.overlay.enabled = false;
    sceneP.hasSkybox = false;
    sceneP.user = this;
    mRenderSystem.scene_pass(sceneP);

    // render UI in screen space
    RenderSystemScreenPass screenP{};
    screenP.callback = &UISandbox::on_screen_render;
    screenP.user = this;
    mRenderSystem.screen_pass(screenP);

    mRenderSystem.submit_frame();
}

void UISandbox::on_event(const WindowEvent* event, void* user)
{
    UISandbox& self = *(UISandbox*)user;

    // pass window events to UI
    UIContext uiCtx = self.mCtx;
    uiCtx.on_window_event((const WindowEvent*)event);
}

void UISandbox::on_screen_render(ScreenRenderComponent renderer, void* user)
{
    UISandbox& self = *(UISandbox*)user;

    UIContext ctx = self.mCtx;

    Vector<UILayer> layers;
    ctx.get_layers(layers);

    for (UILayer layer : layers)
        layer.render(renderer);
}

void UISandbox::on_dialog_render(ScreenRenderComponent renderer, void* user)
{
    UISandbox& self = *(UISandbox*)user;

    renderer.draw_rect(Rect(0, 0, 128, 128), 0xFF0000FF);
}

} // namespace LD