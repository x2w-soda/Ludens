#include <Ludens/DSA/Array.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/Event/WindowEvent.h>
#include <Ludens/JobSystem/JobSystem.h>
#include <Ludens/Log/Log.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/RenderBackend/RStager.h>
#include <Ludens/RenderBackend/RUtil.h>
#include <Ludens/System/FileSystem.h>
#include <Ludens/UI/UIImmediate.h>
#include <Ludens/UI/Widgets.h>
#include <Ludens/WindowRegistry/Input.h>
#include <Ludens/WindowRegistry/WindowRegistry.h>
#include <LudensEditor/EditorContext/EditorIconAtlas.h>
#include <LudensUtil/LudensLFS.h>

#include <string>

#include "UISandbox.h"

#define UI_CONTEXT_NAME "SANDBOX"

namespace LD {

static Log sLog("UISandbox");

static WindowID sWindowID;
static UITextEditData* sTextEdit;

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
    serverI.defaultFontAtlas = mFontAtlas;
    mRenderSystem = RenderSystem::create(serverI);

    mFontAtlasImage = mRenderSystem.get_font_atlas_image();

    {
        Bitmap tmpBitmap = Bitmap::create_from_path(sLudensLFS.editorIconAtlasPath.string().c_str(), false);
        RImageInfo imageI = RUtil::make_2d_image_info(RIMAGE_USAGE_SAMPLED_BIT | RIMAGE_USAGE_TRANSFER_DST_BIT,
                                                      RFORMAT_RGBA8, tmpBitmap.width(), tmpBitmap.height(),
                                                      {.filter = RFILTER_LINEAR, .mipmapFilter = RFILTER_LINEAR, .addressMode = RSAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE});

        mIconAtlasImage = mRDevice.create_image(imageI);

        RStager stager(mRDevice, RQUEUE_TYPE_GRAPHICS);
        stager.add_image_data(mIconAtlasImage, tmpBitmap.data(), RIMAGE_LAYOUT_SHADER_READ_ONLY);
        stager.submit(mRDevice.get_graphics_queue());
        Bitmap::destroy(tmpBitmap);
    }
}

UISandbox::~UISandbox()
{
    LD_PROFILE_SCOPE;

    mRDevice.wait_idle();
    mRDevice.destroy_image(mIconAtlasImage);

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

    UIFontRegistry fontReg = UIFontRegistry::create();
    UIFont font = fontReg.add_font(mFontAtlas, mFontAtlasImage);
    ui_imgui_startup(font, {});

    sTextEdit = heap_new<UITextEditData>(MEMORY_USAGE_UI);

    while (reg.is_window_open(rootID))
    {
        reg.poll_events();

        if (reg.is_window_minimized(rootID))
            continue;

        Vec2 windowExtent = reg.get_window_extent(rootID);

        imgui(windowExtent, (float)reg.get_delta_time());
        render();

        LD_PROFILE_FRAME_MARK;
    }

    heap_delete<UITextEditData>(sTextEdit);

    ui_imgui_cleanup();

    UIFontRegistry::destroy(fontReg);
}

void UISandbox::imgui(const Vec2& windowExtent, float delta)
{
    ui_context_begin(UI_CONTEXT_NAME, windowExtent);

    ui_layer_begin("LAYER");
    ui_workspace_begin("WORKSPACE", Rect(100.0f, 50.0f, 250.0f, 400.0f));

    bool isPressed;

    ui_push_window("WINDOW");
    ui_top_draw([](UIWidget widget, ScreenRenderComponent renderer, void*) {
        renderer.draw_rect(widget.get_rect(), Color(0x303030FF));
    });

    static UIScrollData sScroll;
    ui_push_scroll(&sScroll);
    ui_top_layout_child_gap(10.0f);
    {
        static UIButtonData sBtn1{};
        ui_push_button(&sBtn1, "Button1");
        if (ui_button_is_pressed())
        {
            sLog.info("Button1 pressed!");
        }
        ui_pop();

        static UIButtonData sBtn2{};
        ui_push_button(&sBtn2, "Button2");
        if (ui_button_is_pressed())
        {
            sLog.info("Button2 pressed!");
        }
        ui_pop();

        static UIPanelData sPanel{};
        ui_push_panel(&sPanel);
        {
            UITextWidget text = ui_push_text(nullptr, "Some text1 here!!!!!!");
            if (ui_text_span_pressed(0))
                sLog.info("Text pressed");
            ui_pop();
            static UIImageData sImage{mFontAtlasImage};
            ui_push_image(&sImage, 300, 300);
            ui_pop();
            ui_push_text(nullptr, "Some text2 here!");
            ui_pop();
        }
        ui_pop();

        static float sValue = 2.0f;
        static UISliderData sSlider;
        ui_push_slider(&sSlider, &sValue);
        ui_pop();

        static UIButtonData sBtn3{};
        ui_push_button(&sBtn3, "Button3");
        if (ui_button_is_pressed())
        {
            sLog.info("Button3 pressed!");
        }
        ui_pop();

        std::string text;
        ui_push_text_edit(sTextEdit);
        if (ui_text_edit_changed(text))
            sLog.info("Text Changed: [{}]", text);
        if (ui_text_edit_submitted(text))
            sLog.info("Text Submitted: [{}]", text);
        ui_pop();
    }
    ui_pop();
    ui_pop_window();

    CursorType cursorHint;
    ui_workspace_end();
    ui_layer_end();
    ui_context_end(delta, cursorHint);
}

void UISandbox::render()
{
    WindowRegistry reg = WindowRegistry::get();
    const Vec2 screenExtent = reg.get_window_extent(reg.get_root_id());
    const Viewport screenViewport = Viewport::from_extent(screenExtent);
    const bool hasDialogWindow = reg.is_window_open(sWindowID);

    // begin rendering a frame
    RenderSystemFrameInfo frameI{};
    frameI.directionalLight = Vec3(0.0f, 1.0f, 0.0f);
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

    // render UI in screen space
    RenderSystemScreenPass screenP{};
    screenP.overlay.viewport = screenViewport;
    screenP.overlay.renderCallback = &UISandbox::on_screen_render;
    screenP.user = this;
    mRenderSystem.screen_pass(screenP);

    mRenderSystem.submit_frame();
}

void UISandbox::on_event(const WindowEvent* event, void* user)
{
    UISandbox& self = *(UISandbox*)user;

    // pass window events to UI
    ui_context_input(UI_CONTEXT_NAME, event);
}

void UISandbox::on_screen_render(ScreenRenderComponent renderer, TView<int>, int overlayVPIndex, void* user)
{
    UISandbox& self = *(UISandbox*)user;

    renderer.bind_quad_pipeline(QUAD_PIPELINE_UBER);
    renderer.set_view_projection_index(overlayVPIndex);

    ui_context_render(UI_CONTEXT_NAME, renderer);
}

void UISandbox::on_dialog_render(ScreenRenderComponent renderer, void* user)
{
    UISandbox& self = *(UISandbox*)user;

    renderer.draw_rect(Rect(0, 0, 128, 128), 0xFF0000FF);
}

} // namespace LD