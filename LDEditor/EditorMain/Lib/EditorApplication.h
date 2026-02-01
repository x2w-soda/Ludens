#pragma once

#include <Ludens/AudioServer/AudioServer.h>
#include <Ludens/Media/Font.h>
#include <Ludens/RenderServer/RenderServer.h>
#include <LudensEditor/EditorContext/EditorContext.h>
#include <LudensEditor/EditorUI/EditorUI.h>

namespace LD {

class EditorApplication
{
public:
    EditorApplication();
    EditorApplication(const EditorApplication&) = delete;
    EditorApplication(EditorApplication&&) = delete;
    ~EditorApplication();

    EditorApplication& operator=(const EditorApplication&) = delete;
    EditorApplication& operator=(EditorApplication&&) = delete;

    void run();

private:
    RDevice mRDevice;
    RenderServer mRenderServer;
    AudioServer mAudioServer;
    EditorContext mEditorCtx;
    EditorUI mEditorUI;
    Font mFont;
    FontAtlas mFontAtlas;
    RImage mFontAtlasImage;
    CubemapDataID mEnvCubemap;
};

} // namespace LD