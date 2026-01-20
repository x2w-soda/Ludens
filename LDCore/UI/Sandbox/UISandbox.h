#pragma once

#include <Ludens/Camera/Camera.h>
#include <Ludens/Media/Font.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/RenderServer/RenderServer.h>
#include <Ludens/UI/UIContext.h>

namespace LD {

struct WindowEvent;

/// @brief Sandbox application for experiementing with UI.
class UISandbox
{
public:
    UISandbox();
    ~UISandbox();

    void run();

private:
    void imgui();
    void render();

    static void on_event(const WindowEvent* event, void* user);
    static void on_screen_render(ScreenRenderComponent renderer, void* user);
    static void on_dialog_render(ScreenRenderComponent renderer, void* user);

private:
    Font mFont;
    FontAtlas mFontAtlas;
    RDevice mRDevice;
    RenderServer mRenderServer;
    RImage mFontAtlasImage;
    RImage mIconAtlasImage;
    UIWindow mDemo;
    UIContext mCtx;
    Camera mCamera;
};

} // namespace LD