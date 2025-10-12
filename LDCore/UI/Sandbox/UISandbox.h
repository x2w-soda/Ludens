#pragma once

#include <Ludens/Application/Event.h>
#include <Ludens/Camera/Camera.h>
#include <Ludens/Media/Font.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/RenderServer/RServer.h>
#include <Ludens/UI/UIContext.h>
#include <Ludens/UI/UIWindowManager.h>

namespace LD {

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

    static void on_event(const Event* event, void* user);
    static void on_screen_render(ScreenRenderComponent renderer, void* user);

private:
    Font mFont;
    FontAtlas mFontAtlas;
    RDevice mRDevice;
    RServer mRServer;
    RImage mFontAtlasImage;
    RImage mIconAtlasImage;
    UIWindow mClient;
    UIWindowManager mUIWM;
    Camera mCamera;
};

} // namespace LD